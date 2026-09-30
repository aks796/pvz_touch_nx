# Notes: pvz_touch_nx

Technical notes for the Plants vs. Zombies TV Touch port: what the 32-bit
Switch libraries need, what this game needed, and what helps when porting
other 32-bit Android games.

The process runs in AArch32 mode: the game's libraries are armeabi-v7a, and a
process cannot mix A32 and A64 code. Everything in it is 32-bit: devkitARM,
newlib, libnx32, Mesa, FFmpeg.

---

## 1. Updates the 32-bit libraries need

Each item names the library, the problem, how it showed, and where this port
works around it.

### Status (2026-09-30)

This port builds against [libnx32](https://github.com/aks796/libnx32) branch
`master` at `41b61f92` (version 4.12.0, switchbrew/libnx master merged at
`9b4f3b29`), [mesa32](https://github.com/aks796/mesa32) (devkitPro's
`switch-20.1.0-rc3` branch with the fixes below as commits on top) and
[ffmpeg32](https://github.com/aks796/ffmpeg32).

**Fixed in libnx32.**

| Fixed in | What |
| --- | --- |
| `cb01ef9f` | IPC data that depended on enum size (hid controller list, swkbd, raw enum arguments) |
| `c6c53d20` | `svcSetThreadCoreMask` takes a u64 mask; libnx's `pthread_create` now works on 32-bit |
| `c6c53d20` | `svcGetThreadCoreMask` stack balance |
| `c6c53d20` | New stubs: `svcWaitForAddress` (int32 value in r2, timeout in r3:r4), `svcSignalToAddress`, `svcGetThreadContext3` |
| `c6c53d20` | `armICacheInvalidate` is a function: the R/RX page flip |
| `c6c53d20` | A real `__libnx_exception_entry` (weak, with an optional `__libnx_exception_handler32`) |
| `c6c53d20` | virtmem: u64 region bounds, and 32-bit searches in the code region |
| `c6c53d20` | `__libnx_initheap` clamped to the 1 GiB heap region |
| `c6c53d20` | audout and audin: 64-bit buffer descriptors and u64 tags |
| `c6c53d20` | `envAcquireOwnProcessHandle()` and `nwindowGetDefaultDisplay()` |
| `c6c53d20` | SHA-1, SHA-256 and HMAC for AArch32 |
| `c6c53d20` | fsdev maps 0xE02 to EBUSY, a weak `timespec_get`, and `.rel.dyn` in `switch32.ld` |

**What this port does about those fixes.** It keeps its own hardware-proven
versions until they can be tested on hardware again: `dcr_sched.c`,
`code_flush.c`, `exc32.S`, `nx32_virtmem.c`, `nx_init.c` (heap, `__appInit`,
the default window), the audout layout in `pvz_audio.c`, and the
wait-for-address layout self-test in `bionic_pthread.c`. Its definitions
override the library's at link time. The library's copies are discarded as
unused, and the link map shows no clash.

**Fixed in mesa32.**
- `thrd_success` in `u_thread.h` (`24aa14fe`).
- `eglQuerySurface` sizes (`4e41d89f`). `gl_mesa.c`'s fallback to the window
  size stays, and only applies when the size is 0.
- ETC2 and ASTC on chipset 0x120 (`2c27955c`).
- Render-to-texture without storage (`dddc69a4`).
- An opt-in glthread (`972de9c1`).

`libEGL.a` was rebuilt. `libGLESv1_CM`, which this game uses, is unchanged, as
is ffmpeg32.

**Still open.**
- The crt0 text relocations. `crt0_reloc.c` is still needed.
- The soft-float newlib libm. `bionic_math.c` is still needed.
- miniz's greedy inflate. The fix in `bionic_zlib.c` is still needed.
- libnx's stock `__appInit` aborts on any service failure. `nx_init.c`
  overrides it.
- EGL pbuffers.
- The console and EGL sharing buffer slots.

**Newly running threads.** Since libnx's `pthread_create` now works, Mesa's
worker threads start. Before, their creation failed and Mesa fell back to
doing the work itself. The game's own threads are unaffected: they are made
with `threadCreate` in `bionic_pthread.c`.

**Checked in Ryujinx 1.1.1098 against these libraries (2026-09-30).** The
audout, condition-variable and file self-tests pass, and so does the Mesa test
(program linked, triangle drawn). The game then stops at the known Thumb
`LDREX` gap. Ryujinx's `MapSharedMemory` refuses the time service's shared
memory (`time=d401`), as it did before the update; on hardware it is 0. Not
yet run on hardware with these libraries.

### libnx32 (vita2hos libnx, `Makefile.32`, commit 721c977; fixes above)

**Enums are one byte in IPC data.** devkitARM builds with short enums
(`__ARM_SIZEOF_MINIMAL_ENUM == 1`). Any IPC call that sends an enum, or an
array of one, as raw data sends one byte per value.

- `hidSetSupportedNpadIdType` (called by `padConfigureInput`) sent the
  `HidNpadIdType` list as bytes. Handheld mode worked and wireless controllers
  could not connect.
- The swkbd arguments were shifted by 2 bytes.
- Several raw enum arguments were affected in the same way.

Fixed in the `master` branch of
[libnx32](https://github.com/aks796/libnx32) (commit cb01ef9f). The fix
declares the IPC fields as fixed-width integers, and the dispatch macros
`static_assert` on enum-sized raw data. `tools32/check_short_enums.sh` compares
struct layouts in both enum sizes. Upstream should take the same approach.
Building libnx32 with `-fno-short-enums` instead would change other struct
layouts (NvMap and others) that Mesa and libdrm_nouveau rely on.

**`svcSetThreadCoreMask` declares the mask as `u32`.** The AArch32 SVC ABI
takes the 64-bit mask in r2:r3, so r3 carries whatever the caller left there.
The call fails with InvalidCoreId and the thread stays on the core it was
created on. This port issues the SVC with inline assembly
(`source/dcr_sched.c`).

**`svcGetThreadCoreMask` unbalances the stack.** The stub pushes three words
and restores two. This port issues it with inline assembly
(`source/dcr_sched.c`).

**`svcWaitForAddress` (0x34) has no 32-bit stub.** There are two register
layouts:

- Atmosphère 1.8.0 and later (the value is int64 since 19.0.0): r0 address,
  r1 type, r2:r3 value, r4:r5 timeout.
- The older int32 value: r0 address, r1 type, r2 value, r3:r4 timeout.

On hardware (Atmosphère for firmware 21.x) and on Ryujinx 1.1.1098, timed waits
were only correct with the int32 layout. This port picks the layout with a
self-test at start-up (`source/bionic_pthread.c`, `dcr_pthread_selftest`). A
libnx32 stub should follow whatever layout the kernel it targets uses.

**`armICacheInvalidate` is `(void)0` (`cache.h`).** A 32-bit EL0 thread has no
cache maintenance instructions, so no instruction-cache invalidation ever
happens. Code written at run time (hooks, JIT) can then run stale cache lines.
This port cleans the data cache with `svcFlushProcessDataCache`, then flips a
dedicated AliasCode page R to RX with `svcSetProcessMemoryPermission`. The
kernel invalidates every core's instruction cache when a code page gains or
loses execute permission (`source/code_flush.c`). libnx32 could provide this as
its `armICacheInvalidate`.

**The exception handler is a TODO stub (`exception32.s`,
`__libnx_exception_entry`).** Faults cannot be handled inside the process. This
port overrides it: `source/exc32.S` runs the handler on its own 64 KB stack and
saves r8-r12 and the VFP registers, and `source/exc_handler.c` writes
`crash.log` with module+offset addresses and a stack scan.

**`kernel/virtmem.c` has two 32-bit bugs.**

1. Region ends are computed as `base + size` in `uintptr_t`. A 32-bit process's
   ASLR region ends at 0x1_0000_0000, which wraps to 0, so containment and
   overlap tests at the top of the address space are wrong.
2. `virtmemFindAslr` and `virtmemFindCodeMemory` search the whole ASLR region.
   For 32-bit processes the kernel accepts Shared, Code, AliasCode, SharedCode,
   GeneratedCode, Transfered and ThreadLocal mappings only inside the code
   region, [0x200000, 0x40000000).

This showed as `MapSharedMemory` failing with InvalidCurrentMemory during
`hidInitialize`. `source/nx32_virtmem.c` is a fixed copy of the file: u64
region bounds, and it searches the code region.

**`__nx_dynamic` (crt0) cannot apply text relocations.** devkitARM's prebuilt
target libraries (newlib libc/libm, libsysbase, libstdc++) are not built with
`-fPIC`. Their literal pools hold absolute addresses, so a PIE link has
`R_ARM_RELATIVE` relocations in .text and .rodata. Making a Code page writable
turns it into CodeData on Mesosphère, which can never be executable again (a
boot died with svcBreak 0xDC03).

`source/crt0_reloc.c` replaces `__nx_dynamic` (with `dcr32.specs`,
`dcr32.ld` and `-z notext`):

1. It gets a real handle to its own process by sending the pseudo-handle over
   a session to itself (hbloader's technique).
2. It maps each kernel memory block of .text and .rodata to a writable alias
   with `svcMapProcessMemory`, one block per call. The kernel refuses ranges
   whose blocks differ in state.
3. It applies the relocations through the alias.

Building the 32-bit target libraries with `-fPIC` would remove the need for
this.

**`audout` buffer descriptors.** The audout IPC buffer descriptor has 64-bit
fields for every client, while libnx32's `AudioOutBuffer` has 32-bit pointers.
This port sends append and get-released with the right layout
(`source/pvz_audio.c`, `AoBuf`). libnx32's audout wrapper needs the 64-bit
layout.

**The default window keeps a static `ViDisplay`.** A second
`viOpenDisplay("Default")` fails with AlreadyOpened (0x1272), so the vsync event
could not be fetched separately. `source/nx_init.c` defines
`nwindowGetDefault`, `__nx_win_init` and `__nx_win_exit` itself and shares the
display.

**`exit()` shuts services down while other threads still run.** A game thread
that calls hid after `hidExit` aborts the process (0x1159). This port logs and
ends the process with `svcExitProcess` (`source/bionic_core.c`, `b_exit`). A
32-bit runtime for foreign code needs an exit path that does not tear down
services under running threads.

**Scheduling facts.** These are not bugs, but callers need to know them.

- Only priority 59 (cores 0-2) and priority 63 (core 3) are time-sliced by the
  kernel, every 10 ms.
- Threads created with `threadCreate(..., -2)`, and the main thread, get an
  affinity of their ideal core only.

`source/dcr_sched.c` moves the game's threads to priority 59 on cores 0-2.

### devkitARM newlib (the vita2hos toolchain)

- **libm is a soft-float build.** Every double operation in it is a library
  call. `source/bionic_math.c` implements the hot functions with VFP
  instructions and passes the transcendental ones through.
- **`timespec_get` is declared but not implemented.** Mesa's `c11/threads.h`
  needs it (`source/host_compat.c`).
- **`stat()` opens the file to read its size.** On a file the process holds
  open for writing, this fails with FS result 0xe02. `source/bionic_io.c` sizes
  such files through the open handle.
- **`access()` is unreliable over fsdev.** This port uses `stat()` instead.
- **libnx's fsdev maps most FS results to EIO.** `fsdevGetLastResult` gives the
  real result, and this port logs it.

### zlib: miniz from the toolchain image

miniz's `inflate` is greedy. It consumes all of its input, including the
Adler-32 trailer, into its 32 KB window while output is still owed. libpng
1.5.9 then asks for the next IDAT chunk when `avail_in` is 0, and fails with
"Not enough image data". 244 of the game's 301 PNGs failed in a host
replica.

`source/bionic_zlib.c` hands one consumed byte back when the output is full
and the input is empty, and takes it again on the next call. A real zlib for
the 32-bit toolchain, or this fix in miniz, is needed by any program that uses
libpng on it.

### Mesa 20.1.0-rc3 and libdrm_nouveau 1.0.1 (mesa32)

devkitPro ships these only for AArch64.
[mesa32](https://github.com/aks796/mesa32) (its `./build.sh`) cross-builds
them for AArch32 softfp:

- devkitPro's Switch branch as the base;
- a meson cross file with system `horizon` and cpu_family `arm`;
- `-Wl,-z,notext` for meson's link checks, and `-DHAVE_TIMESPEC_GET`;
- `libGLESv1_CM` built as well, since this game uses OpenGL ES 1.x.

**Short-enum bug.** `mesa_format` is `enum pipe_format`, which is 16 bits wide
under short enums, but it also carries `MESA_ARRAY_FORMAT` values such as
0x80068890. They were truncated, and `glClear` crashed in
`nvc0_screen_is_format_supported`.

The fix is `uint32_t` at the three call sites (`st_format.c`, `glformats.c`,
`formats.c`), in mesa32 `099a02a3` ("AArch32: don't depend on int-sized
enums"). Do not widen the enum itself: that changes bitfield layouts elsewhere.

**EGL.** Mesa's Switch EGL platform offers RGBA8888 window configs without
MSAA, and it rejects Android-only attributes. `source/gl_mesa.c` filters those
attributes and retries without MSAA.

### FFmpeg 7.1.1 (ffmpeg32)

FFmpeg needs int-sized enums, so it is built with `-fno-short-enums`
([ffmpeg32](https://github.com/aks796/ffmpeg32), `./build.sh`, LGPL,
only the mov demuxer and the MPEG-4 and AAC decoders). Code that includes its
headers must be built the same way; this port's Makefile does so for
`pvz_video.c`. It also needs `-Wl,-z,notext`.

### Toolchain and launch setup

- **Toolchain image.** `ghcr.io/vita2hos/devcontainer/vita2hos:latest`:
  devkitARM with patched GCC multilibs, switch-tools, and libnx32 at 721c977.
  `build.sh` mounts the fixed [libnx32](https://github.com/aks796/libnx32)
  (`include/switch`, `switch.h`, `libnx.a` and `libnxd.a` from its
  `prefix/`, or from `DCR_LIBNX32`) over the image's copy. It does not mount
  the whole directory, because the image keeps miniz and deko3d there.
- **Launch method.** A 32-bit program cannot be an NRO: hbloader is 64-bit.
  This port ships a 64-bit launcher NRO that carries the 32-bit ExeFS NSP. When
  it is started from a sphaira forwarder, it installs the NSP as
  `atmosphere/contents/<title id>/exefs.nsp`, with `main.npdm` retargeted to
  that title (`source/dcr_exefs.h`), and restarts the title. An hbl override
  instead needs `override_any_app_address_space=32_bit`.

---

## 2. What this game needed

**The game.** Plants vs. Zombies TV Touch, `com.trans.pvztv` 1.1.5 (builds
260924 and 260925), armeabi-v7a. It has three native modules:

- `libnative_code.so`: Transmension's MobileSDK. Glue similar to
  native_app_glue, the app thread, and the `AG*` platform API.
- `libGameMain.so`: Transmension's port of PopCap's Sexy engine and Plants vs.
  Zombies. Thumb-2, OpenGL ES 1.x.
- `libHomura.so`: the "Touch" mod by ZombieYetis (GPL-3.0). It installs 589
  Cydia Substrate inline hooks, vtable hooks and byte patches in its
  constructor.

`libfmodex` and `libGameRegister` (licensing) are not loaded.

**Loader and hooks: the deferred seal** (`source/pvz_loader.c`). Substrate
makes code RWX to patch it, which Horizon does not allow.

- While libHomura's constructor runs, libGameMain and a trampoline pool are
  writable aliases at their final addresses.
- Then they are remapped as code. Hooking takes 5 ms.
- Byte patches the mod applies later (from its menu) go through a temporary
  alias (`so_patch_code`).
- Substrate's inline `svc #0` cacheflush is patched to `mov r0, #0`, and the
  port flushes the cache itself (`code_flush.c`).

**Constructor order.** libGameMain's static constructors read
`ANDROID_SOURCE_DIR`. They run when `NativeApp::load` dlopens it, after the
environment is set, as Android's loader does.

**kuser helpers.** libgcc's `__sync_*` routines in libnative_code and
libGameMain call the Linux kuser helpers (0xffff0fc0 and neighbours) through
literal pools: 43 cmpxchg and 5 barrier literals in each module. The loader
rewrites those literals to point at `source/kuser.S`.

**Licence check.** The engine's one-time activation server no longer exists,
and a failed activation shuts the engine down after loading. The engine
exempts the device model `IDEA TV`, so `getModel` returns that. The same model
selects the engine's standard gamepad button map.

**Data and load time.** The engine reads its data straight out of the APK zip
(PakLib and zziplib, `<apk>::assets/files`), so nothing is extracted. Loading
took 187 s at first:

- Every sound opened the whole APK again, which is about 0.75 s per open on
  the SD card. The read-only APK is now served from a RAM block cache
  (`dcr_apkcache.c`).
- PakLib probes the files directory for every resource before the APK, and
  each probe is an FS IPC. Missing names are answered from directory listings
  (`dcr_dircache.c`, case-insensitive like FAT).

Loading now takes 18 s.

**Java side.** There is no JVM:

- `jni_core.c` provides the JNI environment.
- `pvz_java.c` has handlers for about 100 methods.
- `FindClass` answers from the class names in the APK's dex files
  (`classes.txt`).
- The UI thread runs a real ALooper with fd callbacks (`android_ndk.c`),
  because libnative_code waits on a pipe.

**Graphics.** Mesa through EGL, OpenGL ES 1.x. The engine sets
`glFrontFace(GL_CW)` with culling enabled, which culls Mesa's `glDrawTex`
quads. Overlays (the intro video, the pointer) are drawn as `glDrawArrays`
quads with the fixed-function state saved and restored (`gl_blit.c`).

**Audio.** The engine pushes 44.1 kHz PCM through the Java `AudioOutput`
class. The port resamples it to 48 kHz for audout (`pvz_audio.c`). Writes
block while three buffers are queued, which paces the mixer as
`AudioTrack.write` does on Android.

**Input.**

- Controllers appear as Android gamepad devices 1 and 2, and the touchscreen
  as device 10.
- The engine's Xbox 360 controller scheme is switched on with the
  `LAWN_GAMEPAD_MODE` environment variable; `Sexy::GetEnv` falls back to
  `getenv`.
- The port always lists two controllers. Game code that needs to know whether
  a second controller is really held checks `Gamepad` status +0x19c: 1 means
  present but never used, 3 means active, 2 means idle for 20 s.

**Networking.** Real libnx BSD sockets serve libHomura's imports only
(`pvz_net.c`), translated from Linux values. Network interfaces come from nifm
(`wlan0`). The engine's own telemetry stays offline.

**The mod, rebuilt.** libHomura is built from its GPL source with Android NDK
r27d (`mod/build_mod.sh`).

- The upstream tags build byte-identical `.text`, `.rodata` and `.data` to the
  official APKs (commits 7f01553 and e91bc9a). This port's changes are the
  commits after `upstream-e91bc9a`.
- The rebuilt library is embedded in the NSP and installed in the game folder.
- It replaces the APK's copy only when the APK carries a known official
  build, checked by CRC.

**English.** 1.1.5 exists only in Chinese. The English builds of the older mod
carry `assets/paks/2.ChangeGameChina.zip`, the Chinese original of every file
their translators changed. `pvz_english.c` builds an English layer in the
files directory, which the engine searches before the APK:

- the pictures and fonts;
- menu signs cut out of the English menu atlas;
- the three string tables, with this port's translations of the newer mod's
  lines.

Fonts need a UTF-8 BOM, and the engine's compiled font cache must be cleared
whenever a font changes.

The layer reads only 148 of the English APK's 2,020 entries.
`tools/make_english_pack.py` records which ones by running the layer on the
host (the host zip reader logs what it extracts, `MINIZ_TRACE`), writes them
to a 21 MB zip, and checks that the layer made from it is byte-identical. The
launcher NRO carries that pack as `romfs:/english.apk`. On the first start
with no English APK in the folder, the game program copies it out as
`PvZ Touch English.apk` (`dcr_setup_english_from_nro`), and from then on it is
an English APK like any other.

**APKs under any name.** Both the game and the English source are
`com.trans.pvztv` with the same engine. `pvz_apks.c` tells them apart by
content: the English builds carry the translation pak and the game does not.

**Memory and threads.**

- About 660-750 MB of the 972 MB heap is in use during play.
- Each module gets a 32 MB reserved region.
- Game threads are libnx threads at priority 59 on cores 0-2.
- Condition variables use `svcWaitForAddress` counters, with 250 ms slices as
  a backstop against missed wakeups.

**Exit.** Android's shutdown order:

1. onPause and onStop;
2. surfaceDestroyed;
3. unloadNativeApp, which pumps work until the app thread exits;
4. `svcExitProcess`, with a 5 s backstop.

**Emulator.** Ryujinx 1.1.1098 runs setup, loading and the self-tests, then
stops in libHomura's first constructor. Its Thumb-2 decoder has no `LDREX`.
Hardware is the real test.

---

## 3. Porting other 32-bit Android games

- **ABI.** armeabi-v7a passes float and double in core registers (softfp).
  Build the host `-mfloat-abi=softfp` so shims and callbacks match without
  per-function annotations.
- **Enum sizes differ.** Android code uses 4-byte enums, and devkitARM
  defaults to 1-byte enums. Declare any struct shared with game code (JNI, NDK
  types, callbacks) with `int` fields, not enums.
- **bionic and newlib differ in layout.** Convert at the boundary:
  - `off_t` is 32-bit in bionic;
  - `timespec` and `timeval` fields are 2 × int32;
  - `mbstate_t` is 4 bytes in bionic and 8 in newlib;
  - `struct statvfs` is 44 bytes on 32-bit bionic;
  - errno numbers are Linux ones;
  - `fnmatch` flags use BSD values.
- **Inventory the imports first.** `tools/gen_imports.py` lists every import of
  the modules and generates the binding table. Log every JNI method with no
  handler rather than failing.
- **Look for code that assumes Linux:**
  - kuser helper literal pools (0xffff0fxx);
  - inline `svc #0` (cacheflush and other syscalls);
  - RWX patching (Substrate, xHook and similar);
  - text relocations;
  - `/proc/self/*` reads (the engine `fread`s `/proc/self/cmdline` unchecked).
- **Every SD card open is an IPC.** Engines that probe many missing files, or
  reopen their APK, need caching.
- **Code memory.** Map it with `svcMapProcessCodeMemory` or aliases. Never make
  a Code page writable: it cannot become executable again. Flush the
  instruction cache as described in section 1.
- **Launching.** Use a forwarder title plus an ExeFS override, with a 64-bit
  launcher NRO that installs it. The NSP can then update itself from a newer
  NRO in its folder (`dcr_setup.c`).
- **Ryujinx 1.1.1098 is useful for setup and loading, but:**
  - its A32 decoder lacks Thumb `LDREX`, VSWP, VADDHN, VSRI, VSLI, VACGT/GE,
    VPADAL, `VSHLL #esize` and fixed-point VCVT;
  - it invalidates translated code only on unmap.
- **Log a lot.** A ring buffer written at exit keeps logging cheap. Record FS
  results, not only errno.
