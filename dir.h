#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef struct _crypto_file {
  char file_name[50];
  char file_path[50];
  int size;
  int permissions;
} crypto_file;

static int dir_getattr(const char *path, struct stat *stbuf);
static int dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int dir_rmfile(const char *path);
static int dir_mkdir(const char *path, mode_t mode);
static int file_open(const char *path, struct fuse_file_info *fi);
static int file_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int main(int argc, char *argv[]);

