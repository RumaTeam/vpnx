#!/usr/bin/env bash
# Builds the release zip (dist/vpnx-<version>.zip): extract it at the root of the SD card.
#   switch/vpnx/vpnx.nro          the app (the Vulkan build)
#   switch/vpnx/assets|scripts    data folders of Visual Pinball
#   switch/vpnx/plugins/scoreview the DMD layouts
# No tables, ROMs or media are included. Needs ./build-vpinball.sh (VARIANT=vk) to have produced build/vpinball-vk.nro.
set -euo pipefail
cd "$(dirname "$0")"
VERSION=${VERSION:-0.1.0}
NRO=${NRO:-build/vpinball-vk.nro}
[ -f "$NRO" ] || { echo "missing $NRO: build it first (VARIANT=vk ./build-vpinball.sh)"; exit 1; }
OUT=dist/vpnx-$VERSION
rm -rf "$OUT" "$OUT.zip"
APP=$OUT/switch/vpnx
mkdir -p "$APP/plugins/scoreview"
cp "$NRO" "$APP/vpnx.nro"
cp -r vpinball/src/assets "$APP/assets"
cp -r vpinball/scripts "$APP/scripts"
cp -r vpinball/plugins/scoreview/layouts "$APP/plugins/scoreview/layouts"
cp vpinball/plugins/scoreview/plugin.cfg "$APP/plugins/scoreview/plugin.cfg"
cp LICENSE NOTICE README.md "$APP/"
find "$OUT" -name '.DS_Store' -delete
# the editor's table templates (blank, example...) are not used on the console and the release carries no tables at all
find "$APP/assets" -maxdepth 1 -name '*.vpx' -delete
cat > "$APP/INSTALL.txt" <<TXT
VPNX $VERSION - Virtual Pinball anywhere

1. Extract this zip at the ROOT of your Switch's SD card (the "switch" folder merges with the one you have).
2. Put your tables (.vpx) in sdmc:/roms/vpx/ and their PinMAME romsets in a "pinmame" folder next to the table
   (for example sdmc:/roms/vpx/pinmame/<romset>.zip). Tables and ROMs are NOT included.
3. Open VPNX from the homebrew menu (sdmc:/switch/vpnx/vpnx.nro). Opened through the album it offers to create a HOME-menu icon.

Read README.md for where every file goes, the controls and troubleshooting.
VPNX is free, open source software (GPL-3.0-or-later) and is not sold. https://github.com/RumaTeam/vpnx
TXT
cat > "$APP/BUILDINFO.txt" <<TXT
version: $VERSION
commit: $(git rev-parse HEAD)
built from: $NRO ($(date -u +%Y-%m-%dT%H:%MZ))
vpnx.nro sha256: $(sha256sum "$NRO" | cut -d' ' -f1)
TXT
(cd "$OUT" && zip -q -9 -r "../vpnx-$VERSION.zip" switch)
rm -rf "$OUT"
( cd dist && sha256sum "vpnx-$VERSION.zip" | tee "vpnx-$VERSION.zip.sha256" )
ls -la "dist/vpnx-$VERSION.zip" | awk '{print "size:", $5}'
