#!/bin/bash

set -e

# Config
DOOM_BIN="/usr/local/bin/fbdoom"
IWAD_DIR="/roms/doom"

# URLs (you may replace with preferred mirrors)
URL_SHAREWARE="https://github.com/nneonneo/universal-doom/raw/refs/heads/main/DOOM1.WAD"
URL_ULTIMATE="https://github.com/Akbar30Bill/DOOM_wads/raw/refs/heads/master/doomu.wad"
URL_DOOM2="https://github.com/Akbar30Bill/DOOM_wads/raw/refs/heads/master/doom2.wad"
URL_PLUTONIA="https://github.com/Akbar30Bill/DOOM_wads/raw/refs/heads/master/plutonia.wad"
URL_TNT="https://github.com/Akbar30Bill/DOOM_wads/raw/refs/heads/master/tnt.wad"
URL_CHEX="https://github.com/Akbar30Bill/DOOM_wads/raw/refs/heads/master/chex.wad"

# Menu
CHOICE=$(dialog --clear \
  --title "DOOM Launcher" \
  --menu "Choose DOOM version:" 13 43 7 \
  1 "DOOM Shareware" \
  2 "DOOM I : The Ultimate Doom" \
  3 "DOOM II: Hell on Earth" \
  4 "DOOM II: The Plutonia Experiment" \
  5 "DOOM II: TNT: Evilution" \
  6 "Chex: Quest" \
  2>&1 >/dev/tty)

case "$CHOICE" in
  1)
    IWAD="$IWAD_DIR/DOOM1.WAD"
    URL="$URL_SHAREWARE"
    ;;
  2)
    IWAD="$IWAD_DIR/DOOMU.WAD"
    URL="$URL_ULTIMATE"
    ;;
  3)
    IWAD="$IWAD_DIR/DOOM2.WAD"
    URL="$URL_DOOM2"
    ;;
  4)
    IWAD="$IWAD_DIR/PLUTONIA.WAD"
    URL="$URL_PLUTONIA"
    ;;
  5)
    IWAD="$IWAD_DIR/TNT.WAD"
    URL="$URL_TNT"
    ;;
  6)
    IWAD="$IWAD_DIR/CHEX.WAD"
    URL="$URL_CHEX"
    ;;
  *)
    clear
    echo "Cancelled."
    exit 0
    ;;
esac

# Download if missing
if [ ! -f "$IWAD" ]; then
  echo "Downloading IWAD..."
  mkdir -p "$IWAD_DIR"
  wget -O "$IWAD" "$URL"
else
  echo "IWAD already exists, skipping download."
fi

# Run DOOM
clear
echo "Launching DOOM with $IWAD..."
exec "$DOOM_BIN" -iwad "$IWAD" "$@"
