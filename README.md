# vpnx — Visual Pinball for the Nintendo Switch

A homebrew (libnx / devkitA64) port of [Visual Pinball](https://github.com/vpinball/vpinball): a `.vpx` table plus
its PinMAME romset, running natively on the console. It is the upstream vpinball (bgfx renderer, OpenGL ES 3.2 through
Mesa/nouveau on the native window) with a thin Switch layer: a libnx shell, a few portability fixes and the
dependencies cross-built as static libraries.

> **This project does not provide, host or link to ROMs, PinMAME romsets, tables (`.vpx`) or any other game files.** You must supply
> your own, legally obtained. See the [Legal notice](#legal-notice).

> If you enjoyed this port, please consider [buying me a coffee](https://ko-fi.com/rumateam).
> This project exists because of the work of many other teams: see [Credits and thanks](#credits-and-thanks).

## Status (work in progress)

| Piece | State |
|---|---|
| Build of `vpinball.nro` | works (devkitPro, LTO link takes ~10 min) |
| Table loading (`.vpx`), physics, VBScript (libwinevbs) | works (a PinMAME table plays) |
| Rendering, OpenGL ES (Mesa/nouveau) | works, ~21 fps |
| Rendering, Vulkan (nxvk) | **works, ~28 fps**; checked on hardware: correct image (colours, textures, lighting), smooth, controls respond |
| PinMAME (libpinmame), audio | ROM loads and runs, audio plays |
| Controls | work with both drivers (Pro Controller / Joy-Con, see below) |
| In-game menu overlay | opens and navigates |
| DMD (ScoreView plugin, embedded top right; follows the Cabinet view) | works; layouts go in `sdmc:/switch/vpnx/plugins/scoreview/` (copy `vpinball/plugins/scoreview/layouts` and `plugin.cfg`) |
| Frontend (standalone) | works: splash, list and grid of the tables with their screenshot, favourites, time played, options, English / Português (see below) |
| Applet-mode notice and forwarder | works: offers an icon in the HOME game list that always opens VPNX in Application mode (code from Ruma) |
| Quitting | `+` in a table returns to the frontend, `B` in the frontend leaves the VPNX; with Ruma, `+` or ZL+ZR+- returns to Ruma; HOME always works |
| Joy-Con gyro nudge, backglass | not done |

## Install and play

You need a Nintendo Switch running custom firmware (Atmosphère) with the homebrew menu (hbmenu), a microSD card, and your own
tables (`.vpx`) and PinMAME romsets. **Tables and ROMs are not included** and are not part of this repository: use files you
are entitled to.

### 1. Install the app and its data

1. Copy the app to `sdmc:/switch/vpnx/vpnx.nro` (the Vulkan build, `vpinball-vk.nro`, renamed; see [Which NRO to run](#which-nro-to-run)).
2. Copy the data folders of Visual Pinball next to it, from `vpinball/` of a checkout (or from a release package):

   | Copy this | To | What it is |
   |---|---|---|
   | `src/assets/` | `sdmc:/switch/vpnx/assets/` | the data files (images, fonts...) the player itself needs |
   | `scripts/` | `sdmc:/switch/vpnx/scripts/` | the VPinMAME helper `.vbs` scripts tables include |
   | `plugins/scoreview/layouts/` and `plugins/scoreview/plugin.cfg` | `sdmc:/switch/vpnx/plugins/scoreview/` | the DMD / score display layouts |

3. Open **VPNX** from hbmenu. Opened through the album (applet mode) it offers to create a **forwarder**, an icon in the HOME game
   list that always opens VPNX in Application mode, with all of the console's memory (recommended: big tables need it). Creating
   it needs the CFW signature patches, like any forwarder. `Y` on that notice continues in applet mode anyway.

### 2. Where to put your files

| What | Where |
|---|---|
| **Tables** (`.vpx`) | `sdmc:/roms/vpx/`, `sdmc:/roms/pinball/` or `sdmc:/switch/vpnx/` (the frontend scans these, one subfolder deep; the subfolders `assets`, `plugins`, `pinmame`, `cache`, `prefs`, `scripts`, `nvram`, `cfg`, `roms` and `music` are skipped) |
| **PinMAME romsets** (`<romset>.zip`, named as the table expects) | in a `pinmame` folder **next to the table**: a table in `sdmc:/roms/vpx/` uses `sdmc:/roms/vpx/pinmame/<romset>.zip` (or `.../pinmame/roms/<romset>.zip`); a table in `sdmc:/switch/vpnx/` uses `sdmc:/switch/vpnx/pinmame/roms/<romset>.zip`. Keep the zip as it is, do not unpack it |
| **Screenshots / media** | `sdmc:/switch/vpnx/media/<table file name>.jpg`, `.jpeg` or `.png`: the name is the `.vpx` file name without the extension (`My Table.vpx` → `media/My Table.jpg`). The frontend shows it in the list and the grid (16:9 looks best). The player saves one automatically **the first time you close the table**; replace it with any image you like |
| **High scores, NVRAM, PinMAME settings** | created by PinMAME in the `pinmame` folder of the table: `nvram/`, `cfg/`, `hi/` |
| **Table-specific scripts / sounds / music** | next to the table, as on the desktop (scripts are searched next to the table first, then in `sdmc:/switch/vpnx/scripts/`) |
| **Settings** | `sdmc:/switch/vpnx/prefs/10.8/VPinballX.ini` (written by the in-game menu) |
| **Frontend state** (favourites, time played, view, language, options) | `sdmc:/switch/vpnx/fe.json` |
| **Logs** | `sdmc:/switch/vpnx/vpinball.log`, `terminate.txt` (uncaught C++ exception), `exception_dump.txt` (crash) |

Resulting layout:

```
sdmc:/switch/vpnx/vpnx.nro                the app (the forwarder opens this path)
sdmc:/switch/vpnx/assets/  scripts/       copied from vpinball
sdmc:/switch/vpnx/plugins/scoreview/      DMD layouts + plugin.cfg
sdmc:/switch/vpnx/media/                  table screenshots
sdmc:/switch/vpnx/prefs/  cache/          settings and caches (created by the app)
sdmc:/switch/vpnx/fe.json                 frontend state
sdmc:/roms/vpx/My Table.vpx               a table...
sdmc:/roms/vpx/pinmame/<romset>.zip       ...and its romset
```

### 3. Play

Pick a table in the frontend (see [Frontend](#frontend-standalone) and [Controls](#controls)). The first load of a table takes a
few seconds (about 7 s for a heavy one with the Vulkan build). Close the table with `+`: the first time, the screenshot is saved
and you are back in the list.

**Troubleshooting**

- *No DMD / score display*: the ScoreView layouts are missing in `sdmc:/switch/vpnx/plugins/scoreview/layouts/`. Tables that draw their own display do not need them.
- *No sound from the game's ROM*: the romset zip is not where the table looks for it (see above) or is the wrong version for the table.
- *A table does not open or the app closes*: open `sdmc:/switch/vpnx/vpinball.log`; it names the stage that failed. A crash also leaves `exception_dump.txt` and an Atmosphère report in `sdmc:/atmosphere/crash_reports/`: include them when reporting a problem.
- *Slow*: see [Performance](#performance); the Vulkan build is the fast one, and docking raises the GPU clock.

## Controls

A launch ball (button) · right stick plunger (pull down, release) · ZL/ZR and L/R flippers · left stick nudge ·
B start · X credit · − in-game menu. In the menu: D-pad navigates, A confirms/increases, B goes back, X cancels the pending
changes. `+` closes the table and returns to the frontend (or to Ruma); HOME always works.

## Frontend (standalone)

Without Ruma the app opens its own browser, drawn in software straight into the libnx framebuffer (no GL, so it does not clash
with the player's Vulkan stack) and in the visual language of Ruma (tokens in `docs/VPNX_WIREFRAME.md`, logo and icon in
`docs/brand/`). It is the same NRO: choosing a table starts the player in the same process; when the table ends the app restarts
into the frontend (`--back`, which skips the splash).

| Button | Action |
|---|---|
| D-pad / left stick | move · ZL/ZR page · L/R previous/next initial |
| A | play · X favourite · − favourites only · Y list/grid |
| + | options: camera view, DMD, console clocks, **language** (English (US) is the default, Português (Brasil)) and **Controls help**, a screen with the buttons in the table, in the in-game menu and in the frontend |
| B | quit |

Tables are found in `/roms/vpx`, `/roms/pinball` and `/switch/vpnx` (one folder deep). The screenshot of a table is
`sdmc:/switch/vpnx/media/<table file name>.jpg|jpeg|png` (the case does not matter; an image whose name is contained in the table's name, or the other way round, also matches); the player saves one the first time a table is closed. The details panel says which file was used, or which name is expected, or why an image could not be read; `sdmc:/switch/vpnx/frontend.log` lists the image found for each table. Favourites, time
played, the view mode and the options are in `sdmc:/switch/vpnx/fe.json`.

Started in applet mode (hbmenu opened from the album) the frontend explains that the player needs Application mode and offers
to create a **forwarder**: an icon in the HOME game list that opens `sdmc:/switch/vpnx/vpnx.nro` in Application mode (the title
is built on the console, which needs the CFW signature patches, as for any forwarder). `Y` goes on in applet mode anyway.

## Graphics drivers: OpenGL ES or Vulkan

The console has two usable graphics drivers and they cannot live in the same binary (both embed Mesa's util code), so
there are two builds of the same sources, differing only in the link:

| | OpenGL ES 3.2 (default build) | Vulkan (`-DVPX_SWITCH_VULKAN=ON`) |
|---|---|---|
| Driver | devkitPro switch-mesa (nouveau) | [nxvk](https://github.com/PalindromicBreadLoaf/nxvk), NVK ported to Horizon, linked statically |
| bgfx | `./build-bgfx.sh` | `BGFX_VARIANT=vk ./build-bgfx.sh` (+ `patches/bgfx-vulkan-nx.patch`) |
| Shaders | GLSL ES compiled by Mesa at start | SPIR-V, precompiled |
| Heavy PinMAME table, handheld | ~21 fps (GPU at 921 MHz) | **~28 fps** (GPU at 461 MHz) |
| Load + static prerender | ~45 s | ~7 s |

nxvk is built in a container (`podman build -t nxvk switch/docker && make` in its repo, ~3 min after the image) and its
`libnvk.a`, `libnvk_support.a` and `libexpat.a` go to `prefix/switch/lib`. Notes from bringing it up: the driver hides the
Tegra X1 (not Vulkan conformant) unless `NVK_I_WANT_A_BROKEN_VULKAN_DRIVER=1` is set; bgfx finds the driver through
`vk_icdGetInstanceProcAddr` (no loader, no dlopen); `VK_KHR_present_wait` is advertised but unusable and is switched off;
newlib gaps the driver needs are filled by `compat/nvk_compat.c`, and `compat/egl_stubs.c` satisfies the EGL calls of the
static SDL.

## Performance

Measured on a Switch in handheld mode with a heavy PinMAME table (~650 draw calls and ~280k triangles per frame, 11
intermediate render targets), at 720p. The app prints a `[perf]` line every 3 s on the nxlink console with the time per frame
of every stage, the bgfx stats (draw calls, triangles, GPU time) and, after 90 s of play, the slowest frames with the time per
script timer. With the profiler build of bgfx (`BGFX_PROFILER=1 BGFX_VARIANT=vk ./build-bgfx.sh`) it also prints a `[scopes]`
line: the time per region of the renderer.

**Where the project stands: ~28 fps with Vulkan, smooth and without lag, which is playable** (the physics runs at 1000 Hz
whatever the frame rate, and the inputs are read every millisecond: the game logic does not get worse at a lower frame rate,
only what is displayed). 60 fps is the target of Visual Pinball (display refresh); the GPU is what limits it, see below.

What was done, in the order it paid off:

| Step | Effect |
|---|---|
| Default clock profile (CPU 1785, memory 1600, GPU 460.8 handheld / 768 docked MHz) instead of the stock homebrew clocks | ~6 -> ~14 fps (GL) |
| Main loop paced to 1 kHz instead of spinning ~900 times per frame, each firing the script's controller-sync timers | script 49 -> 7 ms, ~14 -> ~21 fps (GL) |
| Vulkan (nxvk) instead of OpenGL ES | ~21 -> **~28 fps**, with half the GPU clock; load 45 -> 7 s |

Where the time goes now (Vulkan, handheld), per frame of ~36 ms: the CPU work is ~5 ms (the whole draw loop of ~650 draws is
2.2 ms, descriptor sets 1.5 ms, the draw commands 0.4 ms) and the rest is the CPU waiting for the GPU
(`vkWaitForFences` 25 ms, GPU 22 ms by bgfx's timer). **The app is GPU bound.** With OpenGL ES the same frame cost ~27 ms of
CPU in the driver (~40 us per draw call).

Things tried that did **not** help, so they are not retried blindly:

- Mesa's `glthread` and `MESA_NO_ERROR` on the OpenGL ES build: no change (21.5 fps either way).
- `MaxPrerenderedFrames = 2` (two frames in flight): no change on Vulkan.
- A third swapchain image: no change (the wait is not about the image count).

Not done yet, and the way to go beyond ~28 fps: measure the GPU headroom (`boostmax.txt`, GPU at 921.6 MHz), play docked
(768 MHz), and reduce the GPU work (internal resolution, bloom / ambient occlusion / reflections / motion blur, which the
vpinball settings already offer and which are left at their desktop defaults on purpose).

## Using VPNX from Ruma

[Ruma](https://github.com/RumaTeam/ruma) is the emulation frontend of the same team (libnx + SDL2): a shell that lists the
games of every system and launches each emulator as a *runner*, coming back to the list when the game ends. VPNX is a runner of
Ruma, so the table browser, the covers, the favourites and the per-table options are Ruma's. VPNX does not carry a frontend of
its own for that case.

1. Copy `docs/ruma/runner.json` and `docs/ruma/options.json` to `sdmc:/ruma/runners/vpnx/` and the Vulkan NRO
   (`build/vpinball-vk.nro`) to `sdmc:/ruma/runners/vpnx/vpnx.nro`.
2. Put the tables in `sdmc:/roms/pinball/` (any `.vpx`). They do **not** have to be in `sdmc:/switch/vpnx/`: Ruma passes the
   absolute path of the table, and the helper scripts are found first next to the table, then in the app folder.
3. Keep the game's own data in `sdmc:/switch/vpnx/`: `assets/`, `scripts/` (the VPinMAME helper `.vbs`), `pinmame/roms/` (the
   romsets), `prefs/` and the caches.

What happens when A is pressed on a table in Ruma: Ruma writes `/ruma/session.json` and starts `vpnx.nro --rom <table> --session
<file>`. VPNX reads the table and the resolved options from the session, applies them (view mode, clock profile, DMD), plays, and
when it ends hands control back with `envSetNextLoad(return.nro, return.argv)`. Ruma itself reads and deletes the session (and
counts the playing time), VPNX never touches it. Press **+** (the player's own quit) or hold **ZL+ZR+−**
to leave the game and go back to Ruma; `−` alone is still the player's own menu. VPNX keeps no save states and has no Ruma overlay
(HOME still closes the application). Options (`options.json`, 35): the camera view and cabinet fit, graphics quality (supersampling, MSAA, FXAA, sharpen, ambient occlusion,
reflections, bloom, motion blur, texture size, detail), performance (sync mode, frame limit, prerendered frames, FPS overlay, console
`clock` profile), ball rendering, the two volumes, nudge / tilt / plunger, rumble and the DMD (ScoreView). Every one of them defaults to
`auto`, which writes nothing and leaves the in-game menu / `VPinballX.ini` in charge; the others write the matching `VPinballX.ini` key
(`Player/AAFactor`, `Player/PFReflection`...). The writes persist, so VPNX records what it overrode in `sdmc:/switch/vpnx/ruma_applied.json`
(value found, value written) and puts it back at the next start, before applying the next session: a table's options do not leak into the
next table. A setting the player changed in the game menu in the meantime no longer holds the value VPNX wrote and is left alone. The
shell's own "Show FPS" also turns the FPS overlay on. The rest of the shell's built-in options (aspect, filter, saves...) do not apply to VPNX.
The list is `docs/ruma/options.json`; Ruma keeps an identical copy (`runners/native/vpnx/` in its repository, which also builds and installs
this NRO with `build.sh` / `install.sh`), and `vpinball/switch/main.cpp` has the table that maps each option to its settings.

VPNX is not linked with Ruma's `libruma-runner` (it is built on SDL2, the app on SDL3), so the small part of the protocol that
matters is implemented in `vpinball/switch/main.cpp`.

Without Ruma, the app shows its own frontend (or starts the table given as an argument).

## Which NRO to run

| File | Driver |
|---|---|
| `build/vpinball-vk.nro` | Vulkan (nxvk). **Recommended**: faster, loads in ~7 s |
| `build/vpinball-gl.nro` | OpenGL ES 3.2 (Mesa/nouveau). Fallback |

Send it with `nxlink -a <switch ip> -s <file>.nro` (hbmenu → NetLoader, Y) and leave the game with HOME. Options are files on
the SD card: `noboost.txt` (stock clocks), `boostmax.txt` (top GPU step, 921.6 MHz), `noglthread.txt` (GL build only).

## Development

### Requirements

devkitPro with `switch-dev` and `switch-mesa` (and `devkitA64`), CMake ≥ 3.25, curl, patch, git and python3 (Pillow, only for the
logo / icon scripts in `docs/brand/`). The Vulkan build also needs podman (nxvk is built in a container). The LTO link of the app takes
about 10 minutes.

### How the repository works

The repository does not hold the upstream sources. The `build*.sh` scripts clone or download each dependency at a **pinned commit**
(`patches/*.sha`, `vpinball/platforms/config.sh`) into folders that are git-ignored (`vpinball/`, `pinmame/`, `external/`, `prefix/`,
`build/`) and apply our changes from `patches/*.patch`. What is versioned here is the patches, the build scripts, the Switch layer
and the docs. The Switch layer lives in the patch for vpinball as the new files under `vpinball/switch/`:

| File | What it does |
|---|---|
| `switch/main.cpp` | the libnx shell: logging, watchdog, crash handler, clocks, Ruma session and options, the exit paths |
| `switch/frontend.cpp` / `.h` | the standalone frontend (list, grid, options, splash, applet-mode notice, translations) |
| `switch/forwarder.cpp` / `.h` / `_structs.h` | creation of the HOME-menu forwarder title |
| `forwarder/` (repo root) | payload of that title (NSO/NPDM/NACP, built into `romfs/forwarder/` and embedded in the NRO) |

### Build



```
./build.sh               # libpinmame + pinmame-nro (standalone PinMAME test app)
./build-sdl3.sh          # SDL3 (devkitPro fork)
./build-sdl3-extras.sh   # SDL3_image, SDL3_ttf
./build-bgfx.sh          # bgfx, GLES backend
./build-deps.sh          # libwinevbs, FreeImage, libzip
./build-vpinball.sh      # clones vpinball at patches/vpinball.sha, applies patches/vpinball-switch.patch, builds the GL NRO
(cd forwarder && make)   # only when the forwarder payload changes: rebuilds romfs/forwarder (needs elf2nso, npdmtool, nacptool)
BGFX_VARIANT=vk ./build-bgfx.sh && VARIANT=vk ./build-vpinball.sh   # the Vulkan NRO (needs the nxvk libraries, see below)
```

Send it with `nxlink -a <switch ip> -s build/vpinball-switch/vpinball.nro` (hbmenu → NetLoader, Y).

### Changing the code and saving the patches

Edit the sources under `vpinball/` (or `pinmame/`, `external/...`) and rebuild with `cmake --build build/vpinball-switch -j"$(nproc)"`.
When it works, write the changes back into the repository, **including new files**:

```
cd vpinball && git add -N switch src/renderer/BgfxScopeProfiler.h   # mark new files so the diff includes them
git diff --binary > ../patches/vpinball-switch.patch && git reset -q
cd ../pinmame && git diff src/inptport.c src/libpinmame/libpinmame.cpp src/mame.c > ../patches/pinmame-romspath.patch
```

Then check that the patches still apply on a clean checkout of the pinned commit (`git clone`, `git checkout $(cat patches/vpinball.sha)`,
`git apply --check ../patches/vpinball-switch.patch`) before committing. To move to a newer upstream, update the `.sha` file and fix the
patch conflicts. Changing the forwarder payload: `(cd forwarder && make)` rewrites `romfs/forwarder/`.

### Running and debugging on the console

- Send the NRO with `nxlink -a <switch ip> -s <file>.nro` after pressing Y in hbmenu (the NetLoader listens once per upload). The nxlink
  console shows the app's output: the bgfx traces, a `[perf]` line every 3 s, `[input]`, `[ui]` and `[exit]` markers. It connects only
  for a few seconds after the start, so choose the table quickly (or read `vpinball.log` on the SD card, which has the same lines).
- A crash writes `exception_dump.txt` (`pc`, `lr`, `far`, and `anchor_rt`, the runtime address of the function `CrashAnchor`). Resolve a
  runtime address with `aarch64-none-elf-addr2line -f -C -i -e vpinball.elf <elf address>`, where the ELF address is the runtime address minus
  `anchor_rt` plus the address of `CrashAnchor()` in `aarch64-none-elf-nm -C vpinball.elf`. `terminate.txt` has the call chain of the last C++
  throws. Atmosphère's own reports are in `sdmc:/atmosphere/crash_reports/` (if the first system module to abort is `bsdsocket`, it is a
  consequence, not the cause).
- Things learnt the hard way: libnx has no `pthread_detach`; the process must stop every thread (SDL's audio and timer threads too) before it
  returns to the loader, or the loader crashes; stdout/stderr must not point at a freed console or a closed socket when the app leaves; the
  network service is closed last. The NetLoader and FTP servers of the console give up after a short time: reopen them before each upload.
- There is no CI and no emulator for this target: changes are tested on the hardware.

### Translations

The frontend text is in English; `kPt` in `switch/frontend.cpp` maps each English string to its Portuguese translation (the same table serves
the forwarder messages). To add a language, add a table, an entry in the `lang` option and the lookup in `Tr()`.

## Roadmap

- Backglass (the DMD is done through ScoreView).
- Test the whole flow with Ruma on the console (the runner descriptors are in `docs/ruma/`).
- Frontend: search, per-table options, covers beyond the screenshot.
- 60 fps on heavy tables: see Performance.

## Layout of this repository

- `patches/` — changes to upstream vpinball, PinMAME, SDL3, bgfx and libwinevbs (`vpinball.sha` and `pinmame.sha` pin the upstream commits)
- `compat/` — small libc gaps (newlib) filled in: `posix_memalign`, `timegm`, `waitpid`/`execvp` stubs
- `pinmame-nro/`, `sdl3-test/`, `bgfx-test/` — smoke-test apps used to bring the stack up
- `forwarder/` — payload of the HOME-menu forwarder title (derived from nx-hbloader, ISC); its output is `romfs/forwarder/`, embedded in the NRO
- `docs/` — frontend wireframe and mockup, Ruma runner descriptors (`docs/ruma/`), logo and icon (`docs/brand/`)

Notable libnx findings baked into the patches: no `pthread_detach` (ThreadPool and the vsync wait are changed), pthread
stacks default to 1 MiB (Mesa's GLSL compiler overflows it, so every thread gets 8 MiB), SDL returns no base/pref
path, and bgfx needs the default `NWindow` passed explicitly.

## Legal notice

- **No game files are provided.** VPNX is a player: this repository and its releases contain no ROMs, no PinMAME romsets, no tables
  (`.vpx`), no backglass or media packs, no BIOS or any other copyrighted game content, and we will not help to obtain them. Do not
  ask for them, and do not attach them to issues or pull requests.
- You are responsible for the files you use: dump them from machines you own, or use tables and romsets you have the right to use
  (many tables and romsets are distributed freely by their authors).
- VPNX is an independent homebrew project. It is **not affiliated with or endorsed by** Nintendo, the Visual Pinball or PinMAME projects,
  or the owners of any pinball machine, brand or table. All trademarks belong to their respective owners.
- It runs on consoles with custom firmware, which you modify at your own risk; the software is provided as is, without warranty
  (see the license).

## Credits and thanks

VPNX is a thin layer on top of the work of many people, and it would not exist without them. Thank you to:

- the **[Visual Pinball](https://github.com/vpinball/vpinball)** team and contributors (and everyone behind the table, script and plugin
  ecosystem): the whole player, renderer, physics and plugins are theirs; and to the authors of the **ScoreView** plugin used for the DMD;
- the **[PinMAME / libpinmame](https://github.com/vpinball/pinmame)** developers and, behind them, the MAME team: the emulation of the
  pinball machines' hardware;
- **[libwinevbs](https://github.com/vpinball/libwinevbs)** and the **Wine** project, whose VBScript implementation runs the table scripts;
- **[bgfx](https://github.com/bkaradzic/bgfx)** (Branimir Karadžić and contributors), **[bgfx.cmake](https://github.com/bkaradzic/bgfx.cmake)**,
  and the bgfx fork the vpinball team uses;
- **[devkitPro](https://devkitpro.org)**, **libnx** and the **switchbrew** community (including `switch-examples` and **nx-hbloader**, the
  base of the forwarder payload), and the devkitPro port of **SDL** with **SDL_image** and **SDL_ttf** from the SDL project;
- **Mesa** (the nouveau OpenGL ES driver and **NVK**) and **[nxvk](https://github.com/PalindromicBreadLoaf/nxvk)**, which brings NVK to the
  Switch and made the Vulkan build possible;
- **[dolphin-nx](https://github.com/NaGaa95/dolphin-nx)**, whose forwarder code (MPL-2.0) we adapted;
- **FreeImage** (the toxieainc fork), **libzip**, **FreeType**, **nlohmann/json**, **Atmosphère** and every other open source project
  linked or relied on here;
- **[Ruma](https://github.com/RumaTeam/ruma)**, the emulation frontend this project integrates with.

If you enjoyed this port, please consider [buying me a coffee](https://ko-fi.com/rumateam).

## License

**VPNX is free, open source software and is not sold.** It is released under the **GPL-3.0-or-later** (the text is in `LICENSE`),
like Visual Pinball, whose code the player is built from. The Switch layer of this repository (the patches, `vpinball/switch/`,
`compat/`, the build scripts, the docs and the brand files) is Copyright (c) 2026 RumaTeam under the same license. Our new source
files carry `// license:GPLv3+`, the marker Visual Pinball uses. Anyone who receives the NRO is entitled to its source: every release
points to the exact commit of this repository, and the upstream code is fetched at the commits pinned in `patches/`.

The NRO is free and must stay free. The donation link (Ko-fi) is voluntary and unlocks nothing.

Copies and forks are welcome under the GPL: they must keep the copyright notices, state their changes, stay under the same license and
ship their source. The names **Ruma** and **VPNX** and their logos (including the tile mark in `docs/brand/`) are trademarks of RumaTeam
and are not covered by the GPL: a fork has to use another name and logo. VPNX integrates with [Ruma](https://github.com/RumaTeam/ruma)
(a separate program under its own license). `NOTICE` lists the origin and license of every part.

What is combined into it, each part under its own license (see each project for the full text):

| Component | License |
|---|---|
| Visual Pinball (player, renderer, physics, plugins incl. ScoreView) | GPL-3.0-or-later (a few inherited files keep the older "MAME-like" license noted in its `LICENSE`) |
| PinMAME / libpinmame | BSD-3-Clause for the newer files; the inherited "old MAME" license for the rest: **free of charge, never sold**, and no distribution together with illegal ROM images |
| libwinevbs (from Wine) | LGPL-2.1-or-later |
| bgfx | BSD-2-Clause (the bgfx.cmake build files: CC0) |
| SDL3, SDL3_image, SDL3_ttf | zlib |
| libzip | BSD-3-Clause |
| FreeImage | used under its GPLv3 option |
| Mesa NVK / nxvk | MIT, with GPL-2.0-or-later parts |
| `forwarder.cpp`, `forwarder_structs.h` (from dolphin-nx) | MPL-2.0, adapted; each file keeps its notice |
| Forwarder payload (`forwarder/`, from nx-hbloader) | ISC, see `forwarder/LICENSE.md` |

Because of the PinMAME terms, the project is free of charge and ships no ROMs (see the [Legal notice](#legal-notice)).
