//
// load.c
// loading of external code
//
// (c) 2008 why the lucky stiff, the freelance professor
// (c) 2013 by perl11 org
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "p2.h"
#include "internal.h"
#include "table.h"

void potion_load_code(Potion *P, const char *filename) {
  PN buf, code;
  struct stat stats;
  int fd = -1;
  if (stat(filename, &stats) == -1) {
    fprintf(stderr, "** %s does not exist.", filename);
    return;
  }
  fd = open(filename, O_RDONLY | O_BINARY);
  if (fd == -1) {
    fprintf(stderr, "** could not open %s. check permissions.", filename);
    return;
  }
  buf = potion_bytes(P, stats.st_size);
  if (read(fd, PN_STR_PTR(buf), stats.st_size) == stats.st_size) {
    PN_STR_PTR(buf)[stats.st_size] = '\0';
    code = potion_source_load(P, PN_NIL, buf);
    if (!PN_IS_PROTO(code)) {
      potion_run(P, potion_send(
	  potion_parse(P, buf, (char *)filename),
	  PN_compile, potion_str(P, filename), PN_NIL),
	POTION_JIT);
    }
  } else {
    fprintf(stderr, "** could not read entire file: %s.", filename);
  }
  close(fd);
}

static char *potion_initializer_name(Potion *P, const char *filename, PN_SIZE len) {
  PN_SIZE ext_name_len = 0;
  char *allocated_str, *ext_name, *func_name;
  while (*(filename + ++ext_name_len) != '.' && ext_name_len <= len);
  allocated_str = ext_name = malloc(ext_name_len + 1);
  if (allocated_str == NULL) potion_allocation_error();
  strncpy(ext_name, filename, ext_name_len);
  ext_name[ext_name_len] = '\0';
  ext_name += ext_name_len;
  while (*--ext_name != '/' && ext_name >= allocated_str);
  ext_name++;
  if (asprintf(&func_name, "Potion_Init_%s", ext_name) == -1)
    potion_allocation_error();
  free(allocated_str);
  return func_name;
}

void potion_load_dylib(Potion *P, const char *filename) {
  void *handle = dlopen(filename, RTLD_LAZY);
  void (*func)(Potion *);
  char *err, *init_func_name;
  if (handle == NULL) {
    // TODO: error
    fprintf(stderr, "** error loading %s: %s\n", filename, dlerror());
    return;
  }
  init_func_name = potion_initializer_name(P, filename, strlen(filename));
  func = dlsym(handle, init_func_name);
  err = dlerror();
  free(init_func_name);
  if (err != NULL) {
    fprintf(stderr, "** error loading %s: %s\n", filename, err);
    dlclose(handle);
    return;
  }
  func(P);
}

static PN pn_loader_path;
static const char *pn_loader_extensions[] = {
#ifndef P2
  ".pnb"
  , ".pn"
#else
  ".plc"
  , ".pl"
#endif
  , POTION_LOADEXT
};

static const char *find_extension(char *str) {
  int i;
  PN_SIZE str_len = strlen(str);
  struct stat st;
  for (i = 0;
       i < sizeof(pn_loader_extensions) / sizeof(void *);
       i++) {
    PN_SIZE len = strlen(pn_loader_extensions[i]);
    char buf[str_len + len + 1];
    strncpy(buf, str, str_len);
    strncpy(buf + str_len, pn_loader_extensions[i], len);
    buf[str_len + len] = '\0';
    if (stat(buf, &st) == 0 && S_ISREG(st.st_mode))
      return pn_loader_extensions[i];
  }
  return NULL;
}

char *potion_find_file(char *str, PN_SIZE str_len) {
  char *r = NULL;
  struct stat st;
  PN_TUPLE_EACH(pn_loader_path, i, prefix, {
    PN_SIZE prefix_len = PN_STR_LEN(prefix);
    char dirname[prefix_len + 1 + str_len + 1];
    char *str_pos = dirname + prefix_len + 1;
    char *dot;
    const char *ext;
    memcpy(str_pos, str, str_len);
    dot = memchr(str, '.', str_len);
    if (dot == NULL)
      dirname[sizeof(dirname) - 1] = '\0';
    else
      *dot = '\0';
    memcpy(dirname, PN_STR_PTR(prefix), prefix_len);
    dirname[prefix_len] = '/';
    if (stat(dirname, &st) == 0 && S_ISREG(st.st_mode)) {
      if (asprintf(&r, "%s", dirname) == -1) potion_allocation_error();
      break;
    } else if ((ext = find_extension(dirname)) != NULL) {
      if (asprintf(&r, "%s%s", dirname, ext) == -1) potion_allocation_error();
      break;
    } else {
      char *file;
      if ((file = strrchr(str, '/')) == NULL)
        file = str;
      else
        file++;
      if (asprintf(&r, "%s/%s", dirname, file) == -1) potion_allocation_error();
      if (stat(r, &st) != 0 || !S_ISREG(st.st_mode)) {
        int r_len = prefix_len + 1 + str_len * 2 + 1;
        if ((ext = find_extension(r)) == NULL) { free(r); r = NULL; continue; }
        r = realloc(r, r_len + strlen(ext));
        if (r == NULL) potion_allocation_error();
        strcpy(r + r_len, ext);
      }
      break;
    }
  });
  return r;
}

#ifndef P2
PN potion_load(Potion *P, PN cl, PN self, PN file) {
  char *filename = potion_find_file(PN_STR_PTR(file), PN_STR_LEN(file)), *file_ext;
  if (filename == NULL) {
    fprintf(stderr, "** can't find %s\n", PN_STR_PTR(file));
    return PN_NIL;
  }
  file_ext = filename + strlen(filename);
  while (*--file_ext != '.' && file_ext >= filename);
  if (file_ext++ != filename) {
    if (strcmp(file_ext, "pn") == 0)
      potion_load_code(P, filename);
    else if (strcmp(file_ext, "pnb") == 0)
      potion_load_code(P, filename);
    else if (strcmp(file_ext, POTION_LOADEXT+1) == 0)
      potion_load_dylib(P, filename);
    else
      fprintf(stderr, "** unrecognized file extension: %s\n", file_ext);
  } else {
    fprintf(stderr, "** no file extension: %s\n", filename);
  }
  free(filename);
  return PN_NIL;
}

#else

PN p2_load(Potion *P, PN cl, PN self, PN file) {
  char *filename = potion_find_file(PN_STR_PTR(file), PN_STR_LEN(file)), *file_ext;
  if (filename == NULL) {
    fprintf(stderr, "** can't find %s\n", PN_STR_PTR(file));
    return PN_NIL;
  }
  file_ext = filename + strlen(filename);
  while (*--file_ext != '.' && file_ext >= filename);
  if (file_ext++ != filename) {
    if (strcmp(file_ext, "pl") == 0)
      p2_load_code(P, filename);
    else if (strcmp(file_ext, "plc") == 0)
      p2_load_code(P, filename);
    else if (strcmp(file_ext, POTION_LOADEXT+1) == 0)
      potion_load_dylib(P, filename);
    else
      fprintf(stderr, "** unrecognized file extension: %s\n", file_ext);
  } else {
    fprintf(stderr, "** no file extension: %s\n", filename);
  }
  free(filename);
  return PN_NIL;
}
#endif

void potion_loader_init(Potion *P) {
  pn_loader_path = PN_TUP0();
  PN_PUSH(pn_loader_path, potion_str(P, "lib"));
  PN_PUSH(pn_loader_path, potion_str(P, POTION_PREFIX"/lib/potion"));
  PN_PUSH(pn_loader_path, potion_str(P, "."));

#ifdef P2
#define LOADER_PATH "@INC"
#else
#define LOADER_PATH "LOADER_PATH"
#endif
  potion_define_global(P, potion_str(P, LOADER_PATH), pn_loader_path);
  potion_method(P->lobby, "load", potion_load, "file=S");
}

void potion_loader_add(Potion *P, PN path) {
  PN_PUSH(pn_loader_path, path);
}
