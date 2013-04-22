#include "dir.h"

int num_files = 0;
char *secret_directory = "/home/encrypted/.secret";
unsigned char *password;
char *username;

static struct fuse_operations dir_oper = {
  .getattr = dir_getattr,
  .readdir = dir_readdir,
  .unlink = dir_rmfile,
  .mkdir = dir_mkdir,
  .open = file_open,
  .read = file_read,
  .write = file_write,
  .mknod = file_mknod,
  .release = file_release,
};

static int dir_rmfile(const char *path)
{
  char entry_name[50];
  int ret;
  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Removing file at %s\n", entry_name);fflush(stdout);
  if (unlink(entry_name) == 0)
    return 0;
  return -1;
}

static int dir_mkdir(const char *path, mode_t mode)
{
  char entry_name[50];
  int ret;
  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Making directory at %s\n", entry_name);fflush(stdout);
  if (mkdir(entry_name, mode) == 0)
    return 0;
  return -1;
}

static int dir_getattr(const char *path, struct stat *stbuf)
{

  int ret = 0;
  DIR *d;
  struct dirent *dir;
  char entry_name[50];

  memset(stbuf, 0, sizeof(struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 2;
    return ret;
  } 

  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Doing a lookup for %s.\n", entry_name);

  /* Get attributes from the secret secret_directory */
  ret = stat(entry_name, stbuf);
  if (ret == -1)
    return -errno;
  return 0;
}

static int dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
  DIR *d;
  struct dirent *dir;
  char entry_name[50];
  char private_name[50];
  char *current_directory;
  int found = 0;

  printf("Username is %s.\n", username);
  printf("Path is %s\n", path);
  current_directory = (char *) basename(path);

  if (strcmp(current_directory, "private") == 0 && strstr(path, username) == NULL) {
    printf("User doesn't have access. Hiding all files.\n");
    return 0;
  }
  sprintf(entry_name, "%s%s", secret_directory, path);
  d = opendir(entry_name);
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      printf("Entry: %s\n", dir->d_name);
      sprintf(entry_name, "%s", dir->d_name);
      if (strcmp(entry_name, username) == 0) {
        printf("Found directory.\n");
        found = 1;
        filler(buf, entry_name, NULL, 0);
      } else {
        filler(buf, entry_name, NULL, 0);
      }
    }
    closedir(d);
  } else {
    filler(buf, strerror(errno), NULL, 0);
  }

  /* Only called on a user's first use. */
  if (!found && strcmp(path, "/") == 0) {
    sprintf(entry_name, "%s/%s", secret_directory, username);
    printf("Making directory %s", entry_name);
    sprintf(private_name, "%s/private", entry_name);
    if (mkdir(entry_name, S_IFDIR | 0755) == 0 && mkdir(private_name, S_IFDIR | 0755) == 0) {
      printf("Welcome, %s\nThis is your first time logging in, so we're going to create your home directory now.\n", username);
      return 0;
    } else {
      printf("Failed to build folder: %s.\n", strerror(errno));
      return -errno;
    }
  }
  return 0;
}

static int file_release(const char *path, struct fuse_file_info *fi)
{
  int fd;
  char entry_name[50];
  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Trying to release file: %s\n", entry_name);

  if (strstr(entry_name, "private") != NULL) {
    printf("Releasing private file.\n");
    encrypt(entry_name, password);
  } else {
    printf("Release public file.\n");
  }
  return 0;
}

static int file_open(const char *path, struct fuse_file_info *fi)
{
  int fd;
  char entry_name[50];
  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Trying to open file: %s\n", entry_name);

  fd = open(entry_name, fi->flags);

  if (fd == -1) {
    printf("Failed to open file: %s.\n", strerror(errno));
    return -errno;
  }

  if (strstr(entry_name, "private") != NULL) {
    if (strstr(entry_name, username) == NULL) {
      printf("User doesn't have access.");
      close(fd);
      return 0;
    }
    printf("Private file\n");
    decrypt(entry_name, password);
    printf("Done decrypting.\n");
  } else {
    printf("Public file.\n");
  }

  close(fd);
  return 0;
}

static int file_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  int fd;
  int res;
  (void) fi;
  char entry_name[50];
  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Trying to open file for reading %s", entry_name);
  fd = open(entry_name, O_RDONLY);
  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1) {
    printf("Failed to read file: %s.\n", strerror(errno));
    res = -errno;
  }

  close(fd);
  return res;
}

static int file_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  int fd;
  int res;
  (void) fi;
  char entry_name[50];
  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Trying to open file for writing %s\n", entry_name);
  fd = open(entry_name, O_WRONLY);
  lseek(fd, offset, SEEK_SET);
  if (fd == -1)
    return -errno;

  res = write(fd, buf, size);
  if (res == -1) {
    printf("Failed to write file: %s.\n", strerror(errno));
    res = -errno;
  }

  close(fd);
  return res;
}

static int file_mknod(const char *path, mode_t mode, dev_t dev) {

  char entry_name[50];
  int res;
  sprintf(entry_name, "%s%s", secret_directory, path);
  printf("Trying to create new file %s\n", entry_name);
  res = mknod(entry_name, mode, dev);
  if (res == -1)
    return -errno;
  return res;
}

void encrypt(const char *path, const unsigned char *key) {

  int i;
  aes_encrypt_ctx ctx[1];
  unsigned char iv[16];
  unsigned char temp_name[50];
  unsigned char inBuffer[200], outBuffer[200];
  FILE *dfile;
  FILE *ofile;
  sprintf(temp_name, "%s.tmp", path);
  dfile = fopen(path, "rb");
  ofile = fopen(temp_name, "wb");
  if (dfile != NULL) {
    printf("File.\n");
    fflush(stdout);
  } else {
    return;
  }


  for (i = 0; i < 16; ++i)
    iv[i] = rand() & 0xFF;
  fwrite(iv, 1, 16, ofile);

  aes_encrypt_key256(key, ctx);
  while ((i = fread(inBuffer, 1, sizeof(inBuffer), dfile)) > 0) {
    aes_ofb_crypt(inBuffer, outBuffer, i, iv, ctx);
    fwrite(outBuffer, 1, i, ofile);
  }

  fclose(dfile);
  fclose(ofile);
  if (remove(path) != 0)
    printf("Error removing infile.\n");
  if (rename(temp_name, path) != 0)
    printf("Error renaming file.\n");
}

void decrypt(const char *path, const unsigned char *key) {
  int i;
  aes_encrypt_ctx ctx[1];
  unsigned char iv[16];
  unsigned char temp_name[50];
  unsigned char inBuffer[200], outBuffer[200];
  sprintf(temp_name, "%s.tmp", path);
  FILE *dfile = fopen(path, "rb");
  FILE *ofile = fopen(temp_name, "wb");
  if (dfile != NULL) {
    printf("File.\n");
    fflush(stdout);
  } else {
    return;
  }


  if (fread(iv, 1, 16, dfile) < 16) {
    printf("Decryption error.\n");
    fclose(dfile);
    fclose(ofile);
    return;
  }

  aes_encrypt_key256(key, ctx);
  while ((i = fread(inBuffer, 1, sizeof(inBuffer), dfile)) > 0) {
    aes_ofb_crypt(inBuffer, outBuffer, i, iv, ctx);
    fwrite(outBuffer, 1, i, ofile);
  }

  fclose(dfile);
  fclose(ofile);

  if (remove(path) != 0)
    printf("Error removing encrypted file.\n");
  if (rename(temp_name, path) != 0)
    printf("Error renaming file.\n");

}

int main(int argc, char *argv[])
{
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    printf("Couldn't get current working directory. Exiting.\n");
    return 0;
  } else {
    printf("%s", cwd);
    username = (char *) basename(cwd);
    printf("%s", username);
  }
  password = getpass("Enter your password: ");
  aes_init();
  return fuse_main(argc, argv, &dir_oper);
}
