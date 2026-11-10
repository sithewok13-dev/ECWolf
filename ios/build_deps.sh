#!/bin/bash
# Builds the static iOS (arm64) libraries ECWolf needs into $1: SDL2 (+
# SDL2main), SDL2_mixer (its own built-in decoders only: WAV, Ogg Vorbis
# through stb_vorbis, FLAC through dr_flac, MP3 through minimp3, MIDI
# through Timidity) and SDL2_net. (zlib, bzip2, libjpeg and lzma are built
# with the game, from deps/.) From the owner's UZDoom and Raze ports
# (ios/build_deps.sh).
#
# DEPS_NATIVE=1 builds the same libraries for the machine it runs on (the
# Linux rig).
set -ex
DEPS="$1"
: "${IOS_MIN:=15.0}"
: "${SDL_TAG:=release-2.32.8}"
: "${MIXER_TAG:=release-2.8.1}"
: "${NET_TAG:=release-2.2.0}"
WORK="${RUNNER_TEMP:-/tmp}/ios-deps-src"
rm -rf "$WORK"; mkdir -p "$WORK" "$DEPS"
IOSFLAGS=(-G Ninja -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_MIN" -DCMAKE_BUILD_TYPE=Release
          -DCMAKE_INSTALL_PREFIX="$DEPS" -DBUILD_SHARED_LIBS=OFF
          -DCMAKE_PREFIX_PATH="$DEPS" -DCMAKE_POLICY_VERSION_MINIMUM=3.5)
[ -n "$DEPS_NATIVE" ] && IOSFLAGS=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$DEPS"
          -DBUILD_SHARED_LIBS=OFF -DCMAKE_PREFIX_PATH="$DEPS" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON)
# Homebrew's pkg-config would hand the builds Mac libraries: hide them
export PKG_CONFIG_LIBDIR=/nonexistent PKG_CONFIG_PATH=

lib() {  # lib <name> <git url> <tag> [cmake options...]
  local n=$1 url=$2 tag=$3; shift 3
  git clone -q --depth 1 -b "$tag" "$url" "$WORK/$n"
  # (an iOS build looks for libraries in the iOS SDK only: let these find
  # the ones built here in $DEPS too, but never Homebrew's Mac ones)
  cmake -S "$WORK/$n" -B "$WORK/$n/build" "${IOSFLAGS[@]}" \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH "-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local" "$@"
  cmake --build "$WORK/$n/build"
  cmake --install "$WORK/$n/build"
}

lib SDL https://github.com/libsdl-org/SDL.git "$SDL_TAG" \
  -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF -DSDL_HIDAPI=OFF

lib SDL_mixer https://github.com/libsdl-org/SDL_mixer.git "$MIXER_TAG" \
  -DSDL2_DIR="$DEPS/lib/cmake/SDL2" -DSDL2MIXER_VENDORED=OFF -DSDL2MIXER_DEPS_SHARED=OFF \
  -DSDL2MIXER_SAMPLES=OFF -DSDL2MIXER_INSTALL=ON \
  -DSDL2MIXER_OPUS=OFF -DSDL2MIXER_MOD=OFF -DSDL2MIXER_WAVPACK=OFF -DSDL2MIXER_GME=OFF \
  -DSDL2MIXER_MIDI_FLUIDSYNTH=OFF -DSDL2MIXER_MIDI_TIMIDITY=ON -DSDL2MIXER_MIDI_NATIVE=OFF \
  -DSDL2MIXER_VORBIS=STB -DSDL2MIXER_FLAC=ON -DSDL2MIXER_FLAC_DRFLAC=ON -DSDL2MIXER_FLAC_LIBFLAC=OFF \
  -DSDL2MIXER_MP3=ON -DSDL2MIXER_MP3_MINIMP3=ON -DSDL2MIXER_MP3_MPG123=OFF

lib SDL_net https://github.com/libsdl-org/SDL_net.git "$NET_TAG" \
  -DSDL2_DIR="$DEPS/lib/cmake/SDL2" -DSDL2NET_SAMPLES=OFF -DSDL2NET_INSTALL=ON

test -f "$DEPS/lib/libSDL2.a"
test -f "$DEPS/lib/libSDL2main.a"
test -f "$DEPS/lib/libSDL2_mixer.a"
test -f "$DEPS/lib/libSDL2_net.a"
test -f "$DEPS/include/SDL2/SDL_mixer.h"
test -f "$DEPS/include/SDL2/SDL_net.h"
ls -la "$DEPS/lib"
