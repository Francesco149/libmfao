#!/bin/sh

dir="$(dirname "$0")"
. "$dir"/cflags

tmp=$(mktemp -d)

hide_unnecessary_symbols() {
  [ ! -d "$tmp" ] && echo "W: couldn't find tmp dir" && return
  gcc_syms="$tmp/gcc_exports.sym"
  clang_syms="$tmp/clang_exports.list"
  code="$tmp/main.c"
  echo "int main() { return 0; }" >> "$code"
  exports='mfao_'
  ( printf '{global:'
    for e in $exports; do
      printf '%s;' "$e"
    done
    printf 'local:*;};' ) | sed 's/_;/_*;/g' >"$gcc_syms"
  echo "$exports" | tr ' ' '\n' | sed s/_$/_*/g > "$clang_syms"
  for flags in "-Wl,--version-script=$gcc_syms" \
               "-Wl,-exported_symbols_list,$clang_syms"
  do
    if "$cc" $flags "$code" -o /dev/null >/dev/null 2>&1; then
      ldflags="$ldflags $flags"
      return
    fi
  done
  echo "W: can't figure out how to hide unnecessary symbols"
}

hide_unnecessary_symbols

$cc -shared $cflags "$@" -DMFAO_IMPLEMENTATION mfao.c \
  $ldflags -fpic -o libmfao.so

[ -d "$tmp" ] && rm -rf "$tmp"
