#!/bin/bash


case $1 in
  clean-all)
    gcc_like() {
      local _CC="$1"

      rm -r bld/{dbg,rel,reldbg,asan}/${_CC}/*
    }
    ;;

  setup-all)
    gcc_like() {
      local _CC="$1"
      local _CXX="$2"

      mkdir -p bld/{dbg,rel,reldbg,asan}/${_CC}

      local _CMAKE_FLAGS=( -DCMAKE_MODULE_PATH="/home/daned/.local/share/cmake" -DFORCE_COLORED_DIAGNOSTICS=TRUE -DENABLE_LIBCXX=TRUE )

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/dbg/${_CC}    "${_CMAKE_FLAGS[@]}" -DCMAKE_BUILD_TYPE=Debug
      ninja -C bld/dbg/${_CC} triSYCL

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/rel/${_CC}    "${_CMAKE_FLAGS[@]}" -DCMAKE_BUILD_TYPE=Release
      ninja -C bld/rel/${_CC} triSYCL

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/reldbg/${_CC} "${_CMAKE_FLAGS[@]}" -DCMAKE_BUILD_TYPE=RelWithDebInfo
      ninja -C bld/reldbg/${_CC} triSYCL

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/asan/${_CC}   "${_CMAKE_FLAGS[@]}" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fsanitize=address"
      ninja -C bld/asan/${_CC} triSYCL
    }
    ;;

  build-all)
    gcc_like() {
      local _CC="$1"
      local _CXX="$2"

      ninja -C bld/dbg/${_CC}
      ninja -C bld/rel/${_CC}
      ninja -C bld/reldbg/${_CC}
      ninja -C bld/asan/${_CC}
    }
    ;;

  *)
    echo "$0 [clean-all | setup-all | build-all]"
    ;;
esac

gcc_like gcc   g++
gcc_like clang clang++

