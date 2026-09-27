#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-run}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
APP_NAME="VOX HarmonicEQ"
APP_BUNDLE="$BUILD_DIR/HarmonicEQ_artefacts/Release/Standalone/$APP_NAME.app"
APP_BINARY="$APP_BUNDLE/Contents/MacOS/$APP_NAME"
AU_COMPONENT="$BUILD_DIR/HarmonicEQ_artefacts/Release/AU/$APP_NAME.component"

configure_and_build() {
  if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
  fi
  cmake --build "$BUILD_DIR" --config Release --parallel
  ctest --test-dir "$BUILD_DIR" -C Release --output-on-failure
  xattr -cr "$APP_BUNDLE" "$AU_COMPONENT"
  codesign --force --deep --sign - "$APP_BUNDLE"
  codesign --force --deep --sign - "$AU_COMPONENT"
}

pkill -x "$APP_NAME" >/dev/null 2>&1 || true
configure_and_build

case "$MODE" in
  run)
    /usr/bin/open -n "$APP_BUNDLE"
    ;;
  --debug|debug)
    lldb -- "$APP_BINARY"
    ;;
  --logs|logs)
    /usr/bin/open -n "$APP_BUNDLE"
    /usr/bin/log stream --info --style compact --predicate "process == \"$APP_NAME\""
    ;;
  --telemetry|telemetry)
    /usr/bin/open -n "$APP_BUNDLE"
    /usr/bin/log stream --info --style compact --predicate 'subsystem == "cr.rco.vox.harmoniceq"'
    ;;
  --verify|verify)
    /usr/bin/open -n "$APP_BUNDLE"
    sleep 2
    pgrep -x "$APP_NAME" >/dev/null
    ;;
  --install|install)
    DESTINATION="$HOME/Library/Audio/Plug-Ins/Components"
    LEGACY_COMPONENT="$DESTINATION/Harmonic EQ.component"
    LEGACY_BACKUP="$DESTINATION/Harmonic EQ.component.previous"
    mkdir -p "$DESTINATION"
    if [[ -d "$LEGACY_COMPONENT" && ! -e "$LEGACY_BACKUP" ]]; then
      mv "$LEGACY_COMPONENT" "$LEGACY_BACKUP"
      echo "Archived previous component as $LEGACY_BACKUP"
    fi
    ditto "$AU_COMPONENT" "$DESTINATION/$APP_NAME.component"
    echo "Installed $DESTINATION/$APP_NAME.component"
    ;;
  *)
    echo "usage: $0 [run|--debug|--logs|--telemetry|--verify|--install]" >&2
    exit 2
    ;;
esac
