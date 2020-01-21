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

      local _CMAKE_FLAGS=( -DCMAKE_MODULE_PATH="$(pwd)/cmake;/home/daned/.local/share/cmake" )
      _CMAKE_FLAGS+=( -DENABLE_LIBCXX=TRUE )
      _CMAKE_FLAGS+=( -DFORCE_COLORED_DIAGNOSTICS=TRUE )

      local _CMAKE_FLAGS_REL=( )
      _CMAKE_FLAGS_REL+=( -DOPTIMIZE_FOR_NATIVE=TRUE )
      _CMAKE_FLAGS_REL+=( -DFAST_MATH=TRUE )

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/dbg/${_CC} -DCMAKE_BUILD_TYPE=Debug \
          "${_CMAKE_FLAGS[@]}"
      
      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/rel/${_CC} -DCMAKE_BUILD_TYPE=Release \
          "${_CMAKE_FLAGS[@]}" "${_CMAKE_FLAGS_REL[@]}"

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/reldbg/${_CC} -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          "${_CMAKE_FLAGS[@]}"

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/asan/${_CC} -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          "${_CMAKE_FLAGS[@]}" -DCMAKE_CXX_FLAGS="-fsanitize=address"

      CC=${_CC} CXX=${_CXX} cmake -GNinja -S. -Bbld/asan_dbg/${_CC} -DCMAKE_BUILD_TYPE=Debug \
          "${_CMAKE_FLAGS[@]}" -DCMAKE_CXX_FLAGS="-fsanitize=address"
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

