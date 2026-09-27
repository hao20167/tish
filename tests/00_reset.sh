#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TISH_PATH="$HOME/.tishpath"

rm -rf "$ROOT/tests/output"

if [ -f "$TISH_PATH" ]; then
  tmp=$(mktemp "${TMPDIR:-/tmp}/tishpath.XXXXXX")
  grep -v '/tests/output/bin$' "$TISH_PATH" > "$tmp" || true
  mv "$tmp" "$TISH_PATH"
fi

echo "reset demo output and tish test path"
