#include "ls.h"
#include "builtins.h"
#include "parser.h"
#include "utils.h"
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

static void print_permission(FILE *out, mode_t mode) {
  if (out == NULL) out = stdout;
  fprintf(out, "%c", S_ISDIR(mode) ? 'd' : '-');
  fprintf(out, "%c", (mode & S_IRUSR) ? 'r' : '-');
  fprintf(out, "%c", (mode & S_IWUSR) ? 'w' : '-');
  fprintf(out, "%c", (mode & S_IXUSR) ? 'x' : '-');
  fprintf(out, "%c", (mode & S_IRGRP) ? 'r' : '-');
  fprintf(out, "%c", (mode & S_IWGRP) ? 'w' : '-');
  fprintf(out, "%c", (mode & S_IXGRP) ? 'x' : '-');
  fprintf(out, "%c", (mode & S_IROTH) ? 'r' : '-');
  fprintf(out, "%c", (mode & S_IWOTH) ? 'w' : '-');
  fprintf(out, "%c", (mode & S_IXOTH) ? 'x' : '-');
}

static int handler(FILE *out, Command *cmd) {
  if (out == NULL) out = stdout;
  if (cmd->argc > 2) {
    // TODO: handle more flexible command types
    return COMMAND_FAILED;
  }

  char *path = ".";
  int is_all = 0, is_listing = 0;
  if (cmd->argc >= 1) {
    String *argv = cmd->argv;

    String fst = argv[0];
    if (fst.str[0] != '-') {
      path = fst.str;
    } else {
      for (int i = 0; i < fst.len; i++) {
        if (fst.str[i] == 'a') is_all = 1;
        if (fst.str[i] == 'l') is_listing = 1;
      }
      if (cmd->argc >= 2) {
        path = argv[1].str;
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
        print_permission(out, st.st_mode);
        fprintf(out, " %ld", (long)st.st_nlink);
        fprintf(out, " %s %s", getpwuid(st.st_uid)->pw_name, getgrgid(st.st_gid)->gr_name);
        fprintf(out, " %5ld", (long)st.st_size);
        char buf[64];
        strftime(buf, sizeof(buf), "%b %d %H:%M", localtime(&st.st_mtime));
        fprintf(out, " %s", buf);
        fprintf(out, " %s\n", entry->d_name);
      }
    } else {
      fprintf(out, "%s\t", entry->d_name);
    }
  }
  fprintf(out, "\n");

  closedir(dir);
  
  return COMMAND_SUCESSED;
}

builtin_command_t builtin_ls = {.name = "ls",
                                .description = "list all the files/folders in path "
                                        "$(target)\n$(target) = cwd by default",
                                .handler = handler};
