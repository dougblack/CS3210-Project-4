#include "dir.h"

int num_files = 0;
crypto_file *files;
char *current_directory = "/nethome/dblack7/proj4/.secret";

static struct fuse_operations dir_oper = {
  .getattr = dir_getattr,
  .readdir = dir_readdir,
  .unlink = dir_rmfile,
  .mkdir = dir_mkdir,
  .open = file_open,
  .read = file_read,
};

static int dir_rmfile(const char *path)
{
  char entry_name[50];
  int ret;
  sprintf(entry_name, "%s%s", current_directory, path);
  printf("Removing file at %s\n", entry_name);fflush(stdout);
  if (unlink(entry_name) == 0)
    return 0;
  return -1;
}

static int dir_mkdir(const char *path, mode_t mode)
{
  char entry_name[50];
  int ret;
  sprintf(entry_name, "%s%s", current_directory, path);
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
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return ret;
  } 

  sprintf(entry_name, "%s%s", current_directory, path);
  printf("Doing a lookup for %s.\n", entry_name);

  /* Get attributes from the secret current_directory */
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
  printf("Path is %s.\n", path);
  sprintf(entry_name, "%s%s", current_directory, path);
  d = opendir(entry_name);
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      sprintf(entry_name, "%s", dir->d_name);
      filler(buf, entry_name, NULL, 0);
    }
    closedir(d);
  } else {
    filler(buf, strerror(errno), NULL, 0);
  }
  return 0;
}

static int file_open(const char *path, struct fuse_file_info *fi)
{
  printf("Opening file.\n");  
}

static int file_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
  printf("Reading file.\n");  
}

int main(int argc, char *argv[])
{
  return fuse_main(argc, argv, &dir_oper);
}
