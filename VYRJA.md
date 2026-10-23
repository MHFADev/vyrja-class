# Vyrja Classroom (fork of Veyon 4.11.4)

Vyrja Classroom is a re-branded and UX-improved fork of [Veyon](https://veyon.io)
by **Vyrja Studio**. It is licensed under **GPL-2.0-or-later**, same as upstream
(see `COPYING`). Upstream copyright notices are kept; the About dialog credits Veyon.

## What's different from upstream

| Area | Change |
|---|---|
| Branding | New name (Vyrja Classroom / Vyrja Setup / Vyrja Service in all user-visible text), Vyrja Studio logo as app icon, splash, About, installer bitmaps, `.desktop`, `.ico`. Central constants in `core/src/Branding.h`. |
| Theme | Brand theme (navy `#26236B`, orange `#FF6B4A`, cream `#FAF9FD`) as one stylesheet: `core/resources/vyrja-theme.qss` + `core/src/VyrjaTheme.cpp`. Light and dark variants. |
| Quick actions | **Ctrl+K** command palette in the Master: type "kunci", "lock", "message"… and run any action. Shows whether it applies to selected or all computers. (`master/src/CommandPalette.*`) |
| Live status | Header bar shows online / offline / problem counters (dot, ring, square, so state never depends on color alone), the status bar shows the selection count. Updated every second. |
| Indonesian | `translations/veyon_id.ts` completed for all Master/Configurator/feature UI (LDAP, WebAPI, Windows-service and CLI texts still English). |
| **Quick Setup wizard** | New "Quick Setup" button in Vyrja Setup (opens automatically on first run). *Teacher*: creates the class key, saves a key file for students, scans the network for student PCs (port 11100) and adds them to a room. *Student*: pick the teacher's key file, enable service. No manual key/auth/directory configuration needed. (`configurator/src/SetupWizard.*`) |
| **Quick class** | Toolbar menu + Ctrl+K entries: focus (lock all), unlock, announcement, break with auto-unlock timer, work timer with 5-minute warning, attendance CSV export (Documents/Vyrja Attendance), select online/offline/problem computers, invert/clear selection. (`master/src/ClassScenes.*`) |
| Layout | Toolbar grouped into Monitor / Teach / Files / Power & sessions with separators; empty-state hint when no computers are shown; Master offers to open Quick Setup if no key exists. |
| **UI/UX overhaul** | Brand header bar (mode, timer, clock chips, Ctrl+K launcher), ribbon toolbar with captioned groups, redesigned computer cards (status shape, no shadow), tabs in the bottom bar, branded sidebar + page titles in Vyrja Setup, new wizard look, Indonesian UI. |
| **Design refresh** | Flat navy header with the Vyrja Studio mark, wordmark in empty state / About / wizard, text on orange is navy (AA contrast), 3:1 control borders, visible focus rings, empty state with actions, no hex colors left in C++. Rules and reasons: `branding/DESIGN.md`. |
| **Original icon set** | All ~110 icons redrawn from scratch by `branding/icons/generate_icons.py` (tile / line / badge / state styles, light+dark) – none of the upstream artwork remains. |
| Installer | Windows installer renamed and re-themed (`vyrja-classroom-<ver>-win64-setup.exe`), cloud build workflow in `.github/workflows/vyrja-build.yml`. See **BUILDING.md**. |
| Build | `-DSKIP_PLUGINS="ldap;vncserver"` option to build without git submodules. |

Internal identifiers (namespaces, service ID, config paths, network protocol) are **unchanged**,
so Vyrja Classroom stays compatible with standard Veyon 4.x services on student PCs.

## Building

GitHub release tarballs do **not** contain git submodules (libvncserver, kldap, x11vnc, qthttpserver,
libfakekey, ...). For a full build, clone recursively:

    git clone --recursive <your-repo>

Linux (verified with Qt 6.4 / GCC 13, Ubuntu 24.04):

    cmake -G Ninja -B build -DWITH_TRANSLATIONS=ON -DCMAKE_BUILD_TYPE=Release
    ninja -C build

Without submodules, a reduced build (Master, Configurator, core plugins) works with:

    cmake -G Ninja -B build -DWITH_WEBAPI=OFF -DWITH_TESTS=OFF -DSKIP_PLUGINS="ldap;vncserver" \
          -DCMAKE_CXX_FLAGS="-Wno-error=maybe-uninitialized"

Windows installer: use Veyon's normal mingw cross-build (`ci/` / `nsis/`); not tested here.

## Language

Configurator -> General -> Language, or `veyon-cli config set UI/Language "Bahasa Indonesia (id_ID)"`.
Color scheme: General -> Color scheme (System / Light / Dark).
