# v2/cmake/ios-sim-arm64.cmake
# Native iPadOS / iOS arm64 Simulator toolchain
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_PROCESSOR arm64)
set(CMAKE_OSX_SYSROOT iphonesimulator)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2") # iPhone & iPad
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")

# Disable fixed RECOMP_MEM_BASE for iOS sandbox (must be dynamic arena)
set(RA2_MEM_BASE "" CACHE STRING "host address of the guest's 4 GB (empty: anywhere)")
