
cmake_minimum_required(
  VERSION 3.5
  FATAL_ERROR
)

project(
  prtcl
  VERSION 0.1
  LANGUAGES C CXX
)

include( DECommonDiagnosticFlags RESULT_VARIABLE _FOUND )
include( DELibCXXFlags RESULT_VARIABLE _FOUND )

message( "Common C++ Diagnostic Flags: ${DE_COMMON_DIAGNOSTICS_FLAGS_CXX}" )
message( "LibC++ CXX_FLAGS: ${DE_LibCXX_CXX_FLAGS}" )
message( "LibC++ LINK_FLAGS: ${DE_LibCXX_LINK_FLAGS}" )

