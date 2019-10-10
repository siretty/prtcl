#!/bin/bash

BLD="${1:-asan/clang}"
RUN_COUNT="${RUN_COUNT:-100}"

shift
ARGS=( "$@" )

print_error() {
    tput bold
    tput setab 3
    tput setaf 1
    printf " == ERROR == $1"
    tput sgr0
    printf "\n"
}

if ! ninja -C bld/$BLD ; then
  print_error "BUILD FAILED"
  exit 1
fi

for iteration in $(seq 1 "${RUN_COUNT}") ; do
  if ! ./bld/$BLD/prtcl-tests "${ARGS[@]}" ; then
    print_error "TEST FAILED"
    exit 1
  fi
done

