/*
 * this is free and unencumbered software released into the public domain.
 * refer to the attached UNLICENSE or http://unlicense.org/
 */

#ifndef MFAO_H
#define MFAO_H

typedef struct mfao* mfao_t; /* opaque handle */

char* mfao_version_str();
int mfao_version_major();
int mfao_version_minor();
int mfao_version_patch();
mfao_t mfao_new(void);
void mfao_free(mfao_t m);
void mfao_set_process_name(mfao_t m, char* process_name);
void mfao_add_pattern(mfao_t m, char* pattern);
void mfao_bind_pattern(mfao_t m, char** presult, char* pattern);
void mfao_remove_pattern(mfao_t m, char* pattern);
void mfao_clear_patterns(mfao_t m);
void* mfao_find_patterns(mfao_t m);
void* mfao_result(mfao_t m, char* pattern);
void mfao_clear_results(mfao_t m);
void mfao_add_range(mfao_t m, char* start, char* end);
void mfao_add_range_by_substr(mfao_t m, char* start, char* end);
void mfao_read(mfao_t m, void* addr, void* dst, int n);
int mfao_read_int32(mfao_t m, void* addr);
char* mfao_read_ptr(mfao_t m, void* addr);
char* mfao_read_chain(mfao_t m, int n, void* addr, ...);
void mfao_set_timeout(mfao_t m, int seconds);
int mfao_pid(mfao_t m);

#define MFAO_SILENT_BIT (1<<0) /* no terminal output */
#define MFAO_ALL_MEMORY_BIT (1<<1) /* scan non-executable memory */
void mfao_set(mfao_t m, int mask);
void mfao_clear(mfao_t m, int mask);

#define MFAOEV_NONE 0
#define MFAOEV_PROCESS_CHANGED 1
int mfao_poll_event(mfao_t m);

/*
 * when any call that isn't a mfao_set_* is issued, mfao will wait for a
 * process that matches the configured criteria. currently, process name
 * is the only criteria. once a process is found it is cached and
 * subsequent calls won't hang until the process dies, in which case it
 * will hang waiting for another matching process
 *
 * mfao_poll_event will be notified when things such as process respawn
 * happen. you should call this in a loop and process all events and do
 * all your patterns scans on MFAOEV_PROCESS_CHANGED
 *
 * if any error occurs, all calls are a no-op. errors can be checked with
 * mfao_errno and translated to human readable strings with mfao_strerror
 * or printed to stderr with mfao_perror. you can recover from errors by
 * calling mfao_clear_errno
 *
 * add_pattern uses '?' for wildcards. a single '?' means one entire byte
 * wildcarded, for example: 11 22 33 ? 44 will match
 * { 0x11, 0x22, 0x33, x, 0x44 }. spaces are optional, bytes are always
 * hex and always padded to 2 digits
 * 
 * mfao_find_patterns returns the first result. if you're working with
 * multiple patterns, you should use mfao_result or add patterns with
 * mfao_bind_pattern so the results are stored in the pointers you bind.
 * each pattern stores the first address that matches
 * 
 * *_chain functions will read a multi-level pointer
 * for example, this call
 *
 *   tmp = mfao_read_chain(m, 3, 0x123123, 0x80, 0x40, 0x20);
 *
 * translates to
 *
 *   tmp = mfao_read_ptr(m, 0x123123, 0x80);
 *   tmp = mfao_read_ptr(m, tmp + 0x40);
 *   tmp = mfao_read_ptr(m, tmp + 0x20);
 *
 * (offsets are int)
 *
 * mfao_read_ptr reads 8-byte pointers of a 64-bit process is
 * detected and 4-byte for 32-bit. on a 32-bit build of mfao, you
 * can't use this function on 64-bit processes
 */

#define MFAO_EOK 0
#define MFAO_EOOM 1
#define MFAO_EINVAL 2
#define MFAO_EIO 3
#define MFAO_ETIMEOUT 4
int mfao_errno(mfao_t m);
char* mfao_strerror(int err);
void mfao_perror(mfao_t m, char* msg);
void mfao_clear_errno(mfao_t m);

#endif /* MFAO_H */

/*     this is all you need to know for normal usage. internals below    */
/* ##################################################################### */
/* ##################################################################### */
/* ##################################################################### */

#ifdef MFAO_IMPLEMENTATION
#define pp_stringify_(x) #x
#define pp_stringify(x) pp_stringify_(x)
#define MFAO_VERSION_MAJOR 0
#define MFAO_VERSION_MINOR 0
#define MFAO_VERSION_PATCH 1
#define MFAO_VERSION_STR \
  pp_stringify(MFAO_VERSION_MAJOR) "." \
  pp_stringify(MFAO_VERSION_MINOR) "." \
  pp_stringify(MFAO_VERSION_PATCH)
char* mfao_version_str() { return MFAO_VERSION_STR; }
int mfao_version_major() { return MFAO_VERSION_MAJOR; }
int mfao_version_minor() { return MFAO_VERSION_MINOR; }
int mfao_version_patch() { return MFAO_VERSION_PATCH; }
#undef pp_stringify_
#undef pp_stringify

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#ifndef MFAO_PATTERNS_MAX
#define MFAO_PATTERNS_MAX 128
#endif

#ifndef MFAO_RANGES_MAX
#define MFAO_RANGES_MAX 64
#endif

#ifndef MFAO_QUEUE_MAX
#define MFAO_QUEUE_MAX 64
#endif

typedef struct {
  char* string;
  int* mask;
  unsigned char* bytes;
  int len;
  int cap;
  char* result;
  char** presult;
} pattern_t;

typedef struct { char* start; char* end; } range_t;

struct mfao {
  int flags;
  char* process_name;
  int timeout, ptr_size, error, pid;
  char buf[512];
  char* map_substr;
  range_t matched_map;
  int n_patterns;
  pattern_t patterns[MFAO_PATTERNS_MAX];
  int n_ranges;
  range_t ranges[MFAO_RANGES_MAX];
  int queue[MFAO_QUEUE_MAX], queue_len;
};

void println(mfao_t m, char* fmt, ...) {
  va_list va;
  if (m->flags & MFAO_SILENT_BIT) return;
  va_start(va, fmt);
  vprintf(fmt, va);
  if (!strchr(fmt, '\r')) putchar('\n');
  va_end(va);
}

void print_error(mfao_t m, char* msg) {
  println(m, "%s: %s", msg, strerror(errno));
}

void push_event(mfao_t m, int ev) {
  if (m->queue_len >= MFAO_QUEUE_MAX) {
    println(m, "W: event queue is full, discarding old events");
    m->queue_len = 0;
  }
  m->queue[m->queue_len++] = ev;
}

void mfao_free(mfao_t m) {
  mfao_remove_pattern(m, 0);
  free(m);
}

void mfao_set_process_name(mfao_t m, char* process_name) {
  m->process_name = process_name;
  m->pid = -1;
}

FILE* vfopenf(char* mode, char* fmt, va_list va) {
  char path[512];
  vsprintf(path, fmt, va);
  return fopen(path, mode);
}

FILE* fopenf(char* mode, char* fmt, ...) {
  FILE* f;
  va_list va;
  va_start(va, fmt);
  f = vfopenf(mode, fmt, va);
  va_end(va);
  return f;
}

int read_file(mfao_t m, size_t buf_size, long offset, char* fmt, ...) {
  va_list va;
  FILE* f;
  int n;
  if (!buf_size || buf_size >= sizeof(m->buf)) {
    buf_size = sizeof(m->buf) - 1;
  }
  va_start(va, fmt);
  f = vfopenf("rb", fmt, va);
  va_end(va);
  if (!f) {
    return 0;
  }
  if (fseek(f, offset, SEEK_CUR) == -1) {
    fclose(f);
    return 0;
  }
  n = fread(m->buf, 1, buf_size, f);
  if (!n && ferror(f)) {
    fclose(f);
    return 0;
  }
  m->buf[n] = 0;
  fclose(f);
  return 1;
}

int process_matches(mfao_t m, int pid) {
  if (m->process_name) {
    char* p;
    if (!read_file(m, 0, 0, "/proc/%d/cmdline", pid)) {
      return 0;
    }
    for (p = m->buf + strlen(m->buf) - 1; p > m->buf && *p != '/'; --p);
    ++p;
    if (!strcmp(p, m->process_name)) {
      return 1;
    }
  } else {
    m->error = MFAO_EINVAL;
    println(m, "E: no process matches the criteria specified");
  }
  return 0;
}

void wait_for_process(mfao_t m);

int mfao_poll_event(mfao_t m) {
  wait_for_process(m);
  if (!m->queue_len) {
    return MFAOEV_NONE;
  }
  return m->queue[--m->queue_len];
}

mfao_t mfao_new(void) {
  mfao_t m = calloc(sizeof(struct mfao), 1);
  if (m) {
    m->pid = -1;
  }
  return m;
}

int process_alive(mfao_t m) {
  if (m->pid != -1) {
    if (!process_matches(m, m->pid)) {
      println(m, "%d died", m->pid);
      m->pid = -1;
    }
  }
  return m->pid != -1;
}

void wait_for_process(mfao_t m) {
  int loops, units;
  if (process_alive(m)) return;
  units = m->timeout < 10 ? 1 : m->timeout / 10;
  println(m, "scanning for process...");
  for (loops = 0; ; ++loops) {
    struct dirent* ent;
    DIR* dir = opendir("/proc");
    if (!dir) {
      print_error(m, "opendir");
      m->error = MFAO_EIO;
      return;
    }
    while ((ent = readdir(dir))) {
      int pid;
      if (!isdigit(*ent->d_name)) {
        continue;
      }
      pid = atoi(ent->d_name);
      if (process_matches(m, pid)) {
        m->pid = pid;
        break;
      }
    }
    closedir(dir);
    if (m->error) {
      return;
    } else if (m->pid != -1) {
      m->ptr_size = (int)sizeof(void*);
      if (read_file(m, 5, 0, "/proc/%d/exe", m->pid)) {
        if (*(int*)m->buf == 0x464c457f) {
          if (m->buf[4] == 0x01) m->ptr_size = 4;
        } else {
          println(m, "W: not elf binary");
        }
      }
      break;
    } else if (m->timeout && loops * units >= m->timeout) {
      m->error = MFAO_ETIMEOUT;
      println(m, "E: timed out waiting for process");
      break;
    } else {
      sleep(units);
    }
  }
  println(m, "attached to %d", m->pid);
  println(m, "using %d bytes ptrs", m->ptr_size);
  push_event(m, MFAOEV_PROCESS_CHANGED);
}

#define al_min(x, y) ((x) < (y) ? (x) : (y))
#define al_max(x, y) ((x) > (y) ? (x) : (y))

void for_each_map(mfao_t m, int (*callback)(mfao_t, char*, char*, char*)) {
  FILE* f;
  do {
    wait_for_process(m);
    f = fopenf("r", "/proc/%d/maps", m->pid);
    if (!f) {
      m->pid = -1;
    }
  } while (!f);
  for (;;) {
    char line[512], c = EOF, *start, *end, *p;
    for (p = line; p < line + sizeof(line); *p++ = c) {
      c = fgetc(f);
      if (c == EOF || c == '\n' || ferror(f)) break;
    }
    if (ferror(f)) break;
    *p = 0; p = line; errno = 0;
    start = (char*)strtol(p, &p, 16); ++p;
    end = (char*)strtol(p, &p, 16);
    if (errno) {
      print_error(m, "strtol");
    } else {
      if (callback(m, line, start, end)) break;
    }
    if (c == EOF) break;
  }
  fclose(f);
}

int pattern_callback(mfao_t m, char* line, char* start, char* end) {
  int i, j, max_pattern_len = 0;
  char* p = line;
  p += strcspn(p, " \t");
  p += strspn(p, " \t");
  if (*p != 'r' || (!(m->flags & MFAO_ALL_MEMORY_BIT) && p[2] != 'x')) {
    return 0;
  }
  for (i = 0; i < m->n_ranges; ++i) {
    if ((!end || end > m->ranges[i].start) && start < m->ranges[i].end) {
      break;
    }
  }
  if (i && i >= m->n_ranges) return 0;
  for (j = 0; j < m->n_patterns; ++j) {
    max_pattern_len = al_max(max_pattern_len, m->patterns[j].len);
  }
  for (; start < end; ++start) {
    long off = (long)start;
    unsigned char* b = (void*)m->buf;
    println(m, "%p/%p\033[K\r", start, end);
    max_pattern_len = al_min(max_pattern_len, end - start);
    if (!read_file(m, max_pattern_len, off, "/proc/%d/mem", m->pid)) {
      return 0;
    }
    for (j = 0; j < m->n_patterns; ++j) {
      pattern_t* pat = &m->patterns[j];
      if (end - start < pat->len) continue;
      for (i = 0; i < pat->len; ++i) {
        if (pat->mask[i / 8] & (1<<(i % 8))) {
          pat->bytes[i] = b[i];
          continue; /* wildcard */
        }
        if (b[i] != pat->bytes[i]) { ++i; break; }
      }
      if (b[i - 1] == pat->bytes[i - 1] && !*pat->presult) {
        int k;
        *pat->presult = start;
        println(m, "%p -> %s", start, pat->string);
        for (k = 0; k < m->n_patterns && *m->patterns[k].presult; ++k);
        if (k >= m->n_patterns) return 1;
      }
    }
  }
  return 0;
}

#undef al_min
#undef al_max

void* mfao_find_patterns(mfao_t m) {
  int i;
  for_each_map(m, pattern_callback);
  for (i = 0; i < m->n_patterns; ++i) {
    if (*m->patterns[i].presult) return *m->patterns[i].presult;
  }
  return 0;
}

int xrealloc(mfao_t m, void** p, size_t size) {
  void* tmp = realloc(*p, size);
  if (!tmp) {
    m->error = MFAO_EOOM;
    return 0;
  }
  if (!*p) memset(tmp, 0, size);
  *p = tmp;
  return 1;
}

void mfao_add_pattern(mfao_t m, char* pattern) {
  char* p;
  char byte[3];
  int len;
  pattern_t* pat = &m->patterns[m->n_patterns];
  if (m->n_patterns >= MFAO_PATTERNS_MAX) {
    println(m, "W: pattern cap reached, ignoring");
    m->error = MFAO_EOOM;
    return;
  }
  memset(pat, 0, sizeof(pattern_t));
  for (p = pattern; *p; ++p) {
    if (pat->len >= pat->cap) {
      pat->cap = pat->cap ? pat->cap * 2 : 64;
      if (!xrealloc(m, (void**)&pat->bytes, pat->cap)) return;
      if (!xrealloc(m, (void**)&pat->mask, (pat->cap / 8) * 4)) return;
    }
    if (isspace(*p)) continue;
    if (*p == '?') {
      pat->mask[pat->len / 8] |= 1 << (pat->len % 8);
      ++pat->len;
      continue;
    }
    byte[0] = *p++; byte[1] = *p; byte[2] = 0;
    errno = 0;
    pat->bytes[pat->len] = strtol(byte, 0, 16);
    if (errno) {
      print_error(m, "strtol");
      m->error = MFAO_EINVAL;
      return;
    }
    ++pat->len;
  }
  len = strlen(pattern);
  pat->string = malloc(len);
  if (!pat->string) {
    m->error = MFAO_EOOM;
    return;
  }
  memcpy(pat->string, pattern, len);
  pat->presult = &pat->result;
  ++m->n_patterns;
}

void mfao_bind_pattern(mfao_t m, char** presult, char* pattern) {
  mfao_add_pattern(m, pattern);
  m->patterns[m->n_patterns - 1].presult = presult;
}

void free_pattern(pattern_t* pat) {
  free(pat->string);
  free(pat->mask);
  free(pat->bytes);
}

void mfao_remove_pattern(mfao_t m, char* pattern) {
  int i;
  for (i = 0; i < m->n_patterns; ++i) {
    if (!pattern || !strcmp(m->patterns[i].string, pattern)) {
      free_pattern(&m->patterns[i]);
      memmove(&m->patterns[i], &m->patterns[i + 1],
        (m->n_patterns - i - 1) * sizeof(pattern_t));
    }
  }
}

/* TODO: use a hash table */
void* mfao_result(mfao_t m, char* pattern) {
  int i;
  for (i = 0; i < m->n_patterns; ++i) {
    if (!strcmp(m->patterns[i].string, pattern)) {
      return *m->patterns[i].presult;
    }
  }
  return 0;
}

void mfao_clear_results(mfao_t m) {
  int i;
  for (i = 0; i < m->n_patterns; ++i) {
    *m->patterns[i].presult = 0;
  }
}

void mfao_clear_patterns(mfao_t m) { mfao_remove_pattern(m, 0); }

void mfao_add_range(mfao_t m, char* start, char* end) {
  range_t* range = &m->ranges[m->n_ranges];
  if (m->n_ranges >= MFAO_RANGES_MAX) {
    println(m, "W: ranges cap reached, ignoring");
    m->error = MFAO_EOOM;
    return;
  }
  range->start = start;
  range->end = end;
  ++m->n_ranges;
}

int map_by_substr_callback(mfao_t m, char* line, char* start, char* end) {
  if (strstr(line, m->map_substr)) {
    m->matched_map.start = start;
    m->matched_map.end = end;
    return 1;
  }
  return 0;
}

range_t* map_by_substr(mfao_t m, char* s) {
  m->map_substr = s;
  memset(&m->matched_map, 0, sizeof(m->matched_map));
  if (s) for_each_map(m, map_by_substr_callback);
  return &m->matched_map;
}

void mfao_add_range_by_substr(mfao_t m, char* start_str, char* end_str) {
  char* start = map_by_substr(m, start_str)->start;
  char* end = map_by_substr(m, end_str)->end;
  mfao_add_range(m, start, end);
}

void mfao_read(mfao_t m, void* addr, void* dst, int n) {
  wait_for_process(m);
  if (read_file(m, n, (long)addr, "/proc/%d/mem", m->pid)) {
    /* TODO: avoid memcpy without too much code duplication */
    memcpy(dst, m->buf, n);
  } else {
    m->error = MFAO_EIO;
  }
}

int mfao_read_int32(mfao_t m, void* addr) {
  int res = 0;
  mfao_read(m, addr, &res, 4);
  return res;
}

char* mfao_read_ptr(mfao_t m, void* addr) {
  void* res = 0;
  mfao_read(m, addr, &res, m->ptr_size);
  return res;
}

char* mfao_read_chain(mfao_t m, int n, void* addr, ...) {
  char* value = addr;
  int i;
  va_list va;
  va_start(va, addr);
  for (i = 0; i < n; ++i) {
    value = mfao_read_ptr(m, value + va_arg(va, int));
  }
  va_end(va);
  return value;
}

void mfao_set_timeout(mfao_t m, int seconds) {
  m->timeout = seconds;
}

int mfao_pid(mfao_t m) { return m->pid; }
void mfao_set(mfao_t m, int mask) { m->flags |= mask; }
void mfao_clear(mfao_t m, int mask) { m->flags &= ~mask; }

#define errors(wrap) \
  wrap(MFAO_EOK, "no error") \
  wrap(MFAO_EOOM, "out of memory")\
  wrap(MFAO_EINVAL, "invalid parameter(s)") \
  wrap(MFAO_EIO, "i/o error") \
  wrap(MFAO_ETIMEOUT, "timed out") \

#define error_str(code, str) str "\0"
#define error_code(code, str) code,
char* error_strings = errors(error_str) "unknown error";
char error_codes[] = { errors(error_code) };

int mfao_errno(mfao_t m) { return m->error; }

char* mfao_strerror(int err) {
  int i;
  char* p = error_strings;
  for (i = 0; i < sizeof(error_codes); ++i) {
    if (err == error_codes[i]) {
      break;
    }
    for (; *p; ++p);
    ++p;
  }
  return p;
}

void mfao_perror(mfao_t m, char* msg) {
  fprintf(stderr, "%s: %s\n", msg, mfao_strerror(m->error));
}

void mfao_clear_errno(mfao_t m) { m->error = 0; }

#endif /* MFAO_IMPLEMENTATION */
