#!/bin/bash

BLD="${1:-asan/clang}"
RUN_COUNT="${RUN_COUNT:-100}"
RUN_DEBUG="${RUN_DEBUG:-}"

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

CMD=()
if [ -n "${RUN_DEBUG}" ] ; then
  CMD+=( gdb -q )
fi
CMD+=( "./bld/$BLD/prtcl-tests" )
if [ -n "${RUN_DEBUG}" ] ; then
  QUOTED_ARGS=()
  for i in $(seq 0 $(( ${#ARGS[@]} - 1 )) ) ; do
    QUOTED_ARGS+=( "$(printf "%q" "${ARGS[$i]}")" )
  done
  CMD+=( -ex "run ${QUOTED_ARGS[@]}" )
else
  CMD+=( "${ARGS[@]}" )
fi

echo "${CMD[@]}"

export LSAN_OPTIONS="suppressions=tools/lsan-tracy.supp"

for iteration in $(seq 1 "${RUN_COUNT}") ; do
  if ! "${CMD[@]}" ; then
    print_error "TEST FAILED"
    exit 1
  fi
done

