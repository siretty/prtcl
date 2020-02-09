#!/bin/bash

CC_GCC="${CC_GCC:-gcc}"
CXX_GCC="${CXX_GCC:-g++}"

CC_CLANG="${CC_CLANG:-clang}"
CXX_CLANG="${CXX_CLANG:-clang++}"

case $1 in
  clean-all)
    gcc_like() {
      local _ID="$1"

      rm -r bld/{dbg,rel,reldbg,asan}/${_ID}/*
    }
    ;;

  setup-all)
    gcc_like() {
      local _ID="$1"
      local _CC="$2"
      local _CXX="$3"

      mkdir -p bld/{dbg,rel,reldbg,asan}/${_ID}

      local _CMAKE_FLAGS=( -DCMAKE_MODULE_PATH="$(pwd)/cmake" )
      _CMAKE_FLAGS+=( -DENABLE_LIBCXX=OFF )
      _CMAKE_FLAGS+=( -DFORCE_COLORED_DIAGNOSTICS=ON )

      local _CMAKE_FLAGS_REL=( )
      _CMAKE_FLAGS_REL+=( -DOPTIMIZE_FOR_NATIVE=ON )
      _CMAKE_FLAGS_REL+=( -DFAST_MATH=ON )

      echo "C Compiler:   ${_CC}"
      echo "C++ Compiler: ${_CXX}"

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/dbg/${_ID} -DCMAKE_BUILD_TYPE=Debug \
          "${_CMAKE_FLAGS[@]}"
      
      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/rel/${_ID} -DCMAKE_BUILD_TYPE=Release \
          "${_CMAKE_FLAGS[@]}" "${_CMAKE_FLAGS_REL[@]}"

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/reldbg/${_ID} -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          "${_CMAKE_FLAGS[@]}"

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/asan/${_ID} -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          "${_CMAKE_FLAGS[@]}" -DCMAKE_CXX_FLAGS="-fsanitize=address"

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/asan_dbg/${_ID} -DCMAKE_BUILD_TYPE=Debug \
          "${_CMAKE_FLAGS[@]}" -DCMAKE_CXX_FLAGS="-fsanitize=address"
    }
    ;;

  build-all)
    gcc_like() {
      local _ID="$1"
      local _CC="$2"
      local _CXX="$3"

      ninja -C bld/dbg/${_ID}
      ninja -C bld/rel/${_ID}
      ninja -C bld/reldbg/${_ID}
      ninja -C bld/asan/${_ID}
    }
    ;;

  *)
    echo "$0 [clean-all | setup-all | build-all]"
    ;;
esac

gcc_like gcc   "${CC_GCC}"   "${CXX_GCC}"
gcc_like clang "${CC_CLANG}" "${CXX_CLANG}"

