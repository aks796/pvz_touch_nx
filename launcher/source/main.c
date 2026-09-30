/* pvz_touch_nx.nro -- the launcher: the one file the port ships.
 *
 * The game itself is 32-bit ARM, and a 32-bit program cannot be an NRO
 * (hbloader, which runs NROs, is 64-bit). So the game program -- the wrapper,
 * pvz_nx.nsp -- rides in this NRO's romfs, and is installed on the console as
 * an Atmosphere ExeFS override for the HOME-menu icon it was launched from:
 *
 *   1. the user makes a sphaira forwarder for this NRO and launches it;
 *   2. this runs inside that forwarder title: it writes
 *      /atmosphere/contents/<the forwarder's title id>/exefs.nsp (the wrapper,
 *      main.npdm retargeted to that title id: source/dcr_exefs.h) and
 *      restarts the title;
 *   3. Atmosphere now starts the wrapper for that icon instead of hbloader.
 *      On its first run the wrapper unpacks the game's libraries from the
 *      user's APK (any file name: source/pvz_apks.c tells the APKs apart by
 *      their contents); later NROs update it in place.
 *
 * The override is only ever written for a forwarder (title id 05xx...) that
 * this program is running as -- never for a real game or a system title, and
 * never from hbmenu. MIT.
 */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <switch.h>

#include "dcr_exefs.h"

#define GAME_DIR "sdmc:/switch/pvz_touch_nx"
#define OLD_DIR "sdmc:/switch/pvztouch" /* releases before the rename: moved on the first start */

static PadState g_pad;

static void show(void) { consoleUpdate(NULL); }

/* Waits for + (or the HOME menu closing us). */
static void wait_exit(void) {
  printf("\nPress + to exit.\n");
  while (appletMainLoop()) {
    padUpdate(&g_pad);
    if (padGetButtonsDown(&g_pad) & HidNpadButton_Plus)
      break;
    show();
  }
}

static uint8_t *read_file(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *b = n > 0 ? malloc((size_t)n) : NULL;
  if (b && fread(b, 1, (size_t)n, f) != (size_t)n) {
    free(b);
    b = NULL;
  }
  fclose(f);
  *len = b ? (size_t)n : 0;
  return b;
}

static int write_file(const char *path, const uint8_t *d, size_t len) {
  char tmp[160];
  snprintf(tmp, sizeof tmp, "%s.part", path);
  FILE *f = fopen(tmp, "wb");
  if (!f)
    return -1;
  int ok = fwrite(d, 1, len, f) == len;
  if (fclose(f) != 0)
    ok = 0;
  if (ok) {
    remove(path);
    ok = rename(tmp, path) == 0;
  }
  if (!ok)
    remove(tmp);
  return ok ? 0 : -1;
}

/* The first .apk in `dir` (any name: the game program tells them apart by
 * their contents), into `name`; 0 if there is none. */
static int find_apk(const char *dir, char *name, size_t cap) {
  DIR *d = opendir(dir);
  if (!d)
    return 0;
  int found = 0;
  struct dirent *e;
  while (!found && (e = readdir(d))) {
    size_t n = strlen(e->d_name);
    if (e->d_name[0] == '.' || n < 5 || strcasecmp(e->d_name + n - 4, ".apk"))
      continue; /* "._x.apk": macOS resource forks */
    char path[512];
    struct stat st;
    snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
      snprintf(name, cap, "%s", e->d_name);
      found = 1;
    }
  }
  closedir(d);
  return found;
}

static int install(uint64_t tid) {
  size_t nsp_len = 0, out_len = 0, cur_len = 0;
  uint8_t *nsp = read_file("romfs:/pvz_nx.nsp", &nsp_len), *out = NULL;
  if (!nsp || exefs_build_override(nsp, nsp_len, tid, &out, &out_len)) {
    printf("This launcher's copy of the game program is missing or damaged.\n"
           "Download pvz_touch_nx.nro again.\n");
    free(nsp);
    return -1;
  }
  free(nsp);

  char dir[96], path[128];
  snprintf(dir, sizeof dir, "sdmc:/atmosphere/contents/%016lX", tid);
  snprintf(path, sizeof path, "%s/exefs.nsp", dir);
  uint8_t *cur = read_file(path, &cur_len);
  int same = cur && cur_len == out_len && !memcmp(cur, out, out_len);
  free(cur);
  if (same) {
    /* Already installed, yet this launcher ran instead of the game. */
    printf("The game program is installed for this icon (%s),\n"
           "but Atmosphere started this launcher instead of it.\n\n"
           "Update Atmosphere, then launch the icon again.\n", path);
    free(out);
    return -1;
  }
  mkdir("sdmc:/atmosphere", 0777);
  mkdir("sdmc:/atmosphere/contents", 0777);
  mkdir(dir, 0777);
  int rc = write_file(path, out, out_len);
  free(out);
  if (rc) {
    printf("Could not write %s.\nIs the SD card full or read-only?\n", path);
    return -1;
  }
  printf("Installed the game program for this icon:\n  %s\n", path);
  return 0;
}

int main(int argc, char **argv) {
  consoleInit(NULL);
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&g_pad);
  Result rrc = romfsInit();

  printf("Plants vs. Zombies Touch for Nintendo Switch -- launcher\n"
         "========================================================\n\n");
  size_t blen = 0;
  uint8_t *bnum = R_SUCCEEDED(rrc) ? read_file("romfs:/pvz_nx.build", &blen) : NULL;
  printf("Game program build: %.*s\n", bnum ? (int)(blen && bnum[blen - 1] == '\n' ? blen - 1 : blen) : 7,
         bnum ? (const char *)bnum : "missing");
  free(bnum);

  const char *self = argc > 0 && argv[0] ? argv[0] : "";
  if (*self && !strstr(self, "/switch/pvz_touch_nx/"))
    printf("\nNote: this NRO is at %s.\n"
           "The game files belong in /switch/pvz_touch_nx; keeping the NRO there\n"
           "too lets the game update itself when you replace it.\n", self);

  char apk[256];
  int have_apk = find_apk(GAME_DIR, apk, sizeof apk);
  if (have_apk) {
    printf("\nAPK: %s\n", apk);
  } else if (find_apk(OLD_DIR, apk, sizeof apk)) {
    have_apk = 1; /* the game program moves the old folder over on its first start */
    printf("\nAPK: %s, in /switch/pvztouch (moved to /switch/pvz_touch_nx\n"
           "with your saves and settings when the game starts)\n", apk);
  } else {
    printf("\nAPK: MISSING\n");
  }

  u64 tid = 0;
  svcGetInfo(&tid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
  int forwarder = appletGetAppletType() == AppletType_Application && exefs_is_forwarder_tid(tid);

  if (!forwarder) {
    printf("\nStart this from its own HOME-menu icon:\n"
           "  1. put the APK of your PvZ TV Touch 1.1.5 (com.trans.pvztv, any\n"
           "     file name) in /switch/pvz_touch_nx\n"
           "  2. in sphaira: Homebrew > Plants vs. Zombies Touch > Install Forwarder\n"
           "  3. launch the new Plants vs. Zombies Touch icon on the HOME menu.\n"
           "The first launch installs the game program for that icon and starts it.\n");
    wait_exit();
  } else if (!have_apk) {
    printf("\nCopy the APK of your own PvZ TV Touch (com.trans.pvztv 1.1.5,\n"
           "32-bit ARM / armeabi-v7a) into:\n  %s\n(any file name), then launch this icon again.\n", GAME_DIR);
    wait_exit();
  } else if (install(tid) != 0) {
    wait_exit();
  } else {
    printf("\nStarting Plants vs. Zombies TV Touch...\n"
           "(the first start unpacks the game's libraries from the APK)\n");
    show();
    svcSleepThread(1500000000ll);
    romfsExit();
    Result rc = appletRestartProgram(NULL, 0);
    printf("\nRestarting did not work (0x%x): close this and launch\n"
           "Plants vs. Zombies Touch again.\n", rc);
    wait_exit();
    consoleExit(NULL);
    return 0;
  }
  romfsExit();
  consoleExit(NULL);
  return 0;
}
