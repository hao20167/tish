#include "ls.h"
#include "builtins.h"
#include "parser.h"
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

static void print_permission(mode_t mode) {
  printf("%c", S_ISDIR(mode) ? 'd' : '-');
  printf("%c", (mode & S_IRUSR) ? 'r' : '-');
  printf("%c", (mode & S_IWUSR) ? 'w' : '-');
  printf("%c", (mode & S_IXUSR) ? 'x' : '-');
  printf("%c", (mode & S_IRGRP) ? 'r' : '-');
  printf("%c", (mode & S_IWGRP) ? 'w' : '-');
  printf("%c", (mode & S_IXGRP) ? 'x' : '-');
  printf("%c", (mode & S_IROTH) ? 'r' : '-');
  printf("%c", (mode & S_IWOTH) ? 'w' : '-');
  printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

static int handler(Command *cmd) {
  size_t argc = cmd->argc;
  if (argc > 3) {
    // TODO: handle more flexible command types
    return COMMAND_FAILED;
  }

  char *path = ".";
  int is_all = 0, is_listing = 0;
  if (argc >= 2) {
    char **argv = cmd->argv;

    char *fst = argv[1];
    if (fst[0] != '-') {
      path = fst;
    } else {
      for (int i = 0; i < strlen(fst); i++) {
        if (fst[i] == 'a') is_all = 1;
        if (fst[i] == 'l') is_listing = 1;
      }
      if (argc >= 3) {
        path = argv[2];
      }
    }
  }
  
  DIR *dir = opendir(path);
  if (dir == NULL) {
    perror("tish: ls");
    return COMMAND_FAILED;
  }

  struct stat st;
  char full_path[1024];
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.' && !is_all) continue;
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    if (is_listing) {
      if (lstat(full_path, &st) == 0) {
        print_permission(st.st_mode);
        printf(" %ld", (long)st.st_nlink);
        printf(" %s %s", getpwuid(st.st_uid)->pw_name, getgrgid(st.st_gid)->gr_name);
        printf(" %5ld", (long)st.st_size);
        char buf[64];
        strftime(buf, sizeof(buf), "%b %d %H:%M", localtime(&st.st_mtime));
        printf(" %s", buf);
        printf(" %s\n", entry->d_name);
      }
    } else {
      printf("%s\t", entry->d_name);
    }
  }
  printf("\n");

  closedir(dir);
  
  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_ls = {
  .name = "ls", 
  .description = "list all the files/folders in path $(target)\n$(target) = cwd by default", 
  .handler = handler,
  .run_in_parent = 0
};
