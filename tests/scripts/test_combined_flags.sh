#!/bin/sh
set -e
PROG="build/unix/vncpasswd/vncpasswd"
[ -x "$PROG" ] || { echo "vncpasswd not built"; exit 1; }
DIR=$(printf "secret\n" | "$PROG" -t -f)
[ -f "$DIR/passwd" ] || { echo "Password file missing"; exit 1; }
exit 0
