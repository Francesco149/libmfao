/*
 * displays real-time pp and other info for osu! running under wine
 * this is incomplete, discovering .osu file and ppcalc is TODO
 * 
 * requires https://github.com/Francesco149/oppai-ng
 * 
 * gcc -O3 -I /path/to/libmfao -I /path/to/oppai-ng osu.c -o osu
 * sudo ./osu
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#define MFAO_IMPLEMENTATION
#include "mfao.c"
#define OPPAI_IMPLEMENTATION
#include "oppai.c"

void mods_str(char* buf, int mods) {
  if (!mods) {
    strcpy(buf, "nomod");
  } else {
    char* p = buf;
    *p++ = '+';
    #define m(x) if (mods & MODS_##x) p += sprintf(p, #x);
    m(NF) m(EZ) m(TD) m(HD) m(HR) m(SD) m(DT) m(RX) m(HT) m(NC) m(FL)
    m(AT) m(SO) m(AP) m(PF) m(NOMOD)
    #undef m
  }
}

char buf[512], *songs;
/* define HARDCODED to use these values from b20190226.2 (stable)
 * instead of scanning */
char *beatmap = (char*)0x2512858, *beatmap_asm;
char *time = (char*)0x3cf5d90, *time_asm;
char *ingame = (char*)0x2513ed4, *ingame_asm;
int value, salt, mods, id, set_id, music_time, combo;
double acc;

char* find_songs_folder(mfao_t m) {
  FILE* f;
  size_t line_len = 0;
  char* line = 0;
  char* res = 0;
  sprintf(buf, "/proc/%d/maps", mfao_pid(m));
  f = fopen(buf, "r");
  if (!f) { perror("fopen"); return 0; }
  while (getline(&line, &line_len, f) != -1) {
    if (strstr(line, "osu!.exe")) {
      char* p = line + strlen(line) - 1;
      for ( ; p > line && *p != '/'; *p-- = 0);
      if (*p == '/') ++p;
      strcpy(p, "Songs");
      for ( ; p > line && !isspace(*p); --p);
      ++p;
      printf("songs folder: %s\n", p);
      res = strdup(p);
      break;
    }
  }
  free(line);
  fclose(f);
  return res;
}

int main() {
  mfao_t m = mfao_new();
  mfao_set_process_name(m, "osu!.exe");
  mfao_add_range_by_substr(m, "OpenTK.dll", "mscoreei.dll");
  mfao_add_range_by_substr(m, "mscorrc.dll", "OpenTK.dll");
  mfao_bind_pattern(m, &ingame_asm,
    "8B 0D ? ? ? ? 85 C9 74 ? 8D ? ? 8B ? 8B ? ? FF ? ? D9");
  mfao_bind_pattern(m, &ingame_asm, "8D 15 ? ? ? ? E8 ? ? ? ? 8D ? ? "
      "E8 ? ? ? ? 8B 83 ? ? ? ? 8D 56 ? E8 ? ? ? ? 8B 83 ? ? ? ? 8D 56 ? "
      "E8 ? ? ? ? 8B 83");
  mfao_bind_pattern(m, &beatmap_asm,
    "8B 35 ? ? ? ? 85 F6 74 ? 8B ? ? ? ? ? ? E8 ? ? ? ? 83 F8 05");
  mfao_bind_pattern(m, &time_asm, "A1 ? ? ? ? A3 ? ? ? ? FF 15 ? ? ? ? 83");
  for (;;) {
    int ev;
    while ((ev = mfao_poll_event(m))) {
      if (ev == MFAOEV_PROCESS_CHANGED) {
        free(songs);
        songs = find_songs_folder(m);
      #ifndef HARDCODED
      again:
        mfao_clear_results(m);
        mfao_clear_errno(m);
        if (mfao_find_patterns(m)) {
          beatmap = mfao_read_ptr(m, beatmap_asm + 2);
          ingame = mfao_read_ptr(m, ingame_asm + 2);
          time = mfao_read_ptr(m, time_asm + 1);
          if (mfao_errno(m)) goto again;
          printf("beatmap: %p at %p\n", beatmap, beatmap_asm);
          printf("ingame: %p at %p\n", ingame, ingame_asm);
          printf("time: %p at %p\n", time, time_asm);
        } else {
          puts("scanning failed, retrying in 1s");
          sleep(1);
          goto again;
        }
      #endif
      }
    }
    value = mfao_read_chain(m, 4, ingame, 0, 0x38, 0x1c, 0x08);
    salt = mfao_read_chain(m, 4, ingame, 0, 0x38, 0x1c, 0x0c);
    mods = value ^ salt;
    id = mfao_read_chain(m, 2, beatmap, 0, 0xc0);
    set_id = mfao_read_chain(m, 2, beatmap, 0, 0xc4);
    music_time = mfao_read_int32(m, time);
    combo = mfao_read_chain(m, 3, ingame, 0, 0x34, 0x18);
    mfao_read(m, mfao_read_chain(m, 2, ingame, 0, 0x48) + 0x14, &acc, 8);
    mods_str(buf, mods);
    printf("[%d] /b/%d /s/%d %g%% %dx %s\033[K\r",
      music_time, id, set_id, acc, combo, buf);
    mfao_clear_errno(m);
  }
  return 0;
}
