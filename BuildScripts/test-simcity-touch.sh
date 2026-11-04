#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -I Limelight/Input Limelight/Input/SimCityTouchCore.c Tests/SimCityTouchCoreTests.c \
  -lm -o "$test_dir/simcity-touch-tests"
"$test_dir/simcity-touch-tests"
