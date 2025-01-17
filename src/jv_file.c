#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jv.h"
#include "jv_unicode.h"

jv jv_load_file(const char* filename, int raw) {
  struct stat sb;
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    return jv_invalid_with_msg(jv_string_fmt("Could not open %s: %s",
                                             filename,
                                             strerror(errno)));
  }
  if (fstat(fd, &sb) == -1 || S_ISDIR(sb.st_mode)) {
    close(fd);
    return jv_invalid_with_msg(jv_string_fmt("Could not open %s: %s",
                                             filename,
                                             "It's a directory"));
  }
  FILE* file = fdopen(fd, "r");
  struct jv_parser* parser = NULL;
  jv data;
  if (!file) {
    close(fd);
    return jv_invalid_with_msg(jv_string_fmt("Could not open %s: %s",
                                             filename,
                                             strerror(errno)));
  }
  if (raw) {
    data = jv_string("");
  } else {
    data = jv_array();
    parser = jv_parser_new(0);
  }

  char buf[4096];
  while (!feof(file) && !ferror(file)) {
    size_t n = fread(buf, 1, sizeof(buf), file);
    int len = 0;

    if (n == 0)
      continue;

    if (raw) {
      data = jv_string_append_buf(data, buf, n);
    } else {
      jv_parser_set_buf(parser, buf, n, !feof(file));
      jv value;
      while (jv_is_valid((value = jv_parser_next(parser))))
        data = jv_array_append(data, value);
      if (jv_invalid_has_msg(jv_copy(value))) {
        jv_free(data);
        data = value;
        break;
      }
    }
  }
  if (!raw)
      jv_parser_free(parser);
  int badread = ferror(file);
  if (fclose(file) != 0 || badread) {
    jv_free(data);
    return jv_invalid_with_msg(jv_string_fmt("Error reading from %s",
                                             filename));
  }
  return data;
}
