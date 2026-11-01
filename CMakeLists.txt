# =============================================================================
# Red Alert 2 / Yuri's Revenge - Native iPadOS / iOS arm64 static recompilation
# =============================================================================
cmake_minimum_required(VERSION 3.20)

# RA2_TARGET: "game" (Red Alert 2) or "gamemd" (Yuri's Revenge)
set(RA2_TARGET "game" CACHE STRING "Target binary to build: 'game' (Red Alert 2) or 'gamemd' (Yuri's Revenge)")
set(BUNDLE_ID_PREFIX "com.ra2port" CACHE STRING "Prefix for Bundle Identifier")
set(DEVELOPMENT_TEAM "" CACHE STRING "Apple Developer Team ID")
set(CODE_SIGN_STYLE "Automatic" CACHE STRING "Code signing style (Automatic / Manual)")

if(RA2_TARGET STREQUAL "game")
  project(RedAlert2 C OBJC)
  set(TARGET_NAME RedAlert2)
  set(APP_NAME "Red Alert 2")
  set(APP_EXE "game.exe")
  set(APP_ICON_NAME "AppIcon")
  set(BUNDLE_ID "${BUNDLE_ID_PREFIX}.redalert2")
  set(GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/recomp/gen_game")
  set(INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/ios/Info-ra2.plist")
  set(TARGET_ASSETS
    "ra2.mix"
    "language.mix"
    "maps01.mix"
    "maps02.mix"
    "movies01.mix"
    "movies02.mix"
    "multi.mix"
    "theme.mix"
    "game.exe"
    "RA2.INI"
    "Blowfish.dll"
    "BINKW32.DLL"
  )
else()
  project(YurisRevenge C OBJC)
  set(TARGET_NAME YurisRevenge)
  set(APP_NAME "Yuri's Revenge")
  set(APP_EXE "gamemd.exe")
  set(APP_ICON_NAME "AppIcon-YR")
  set(BUNDLE_ID "${BUNDLE_ID_PREFIX}.yurisrevenge")
  set(GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/recomp/gen_yr")
  set(INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/ios/Info-yr.plist")
  set(TARGET_ASSETS
    "ra2.mix"
    "language.mix"
    "ra2md.mix"
    "langmd.mix"
    "expandmd01.mix"
    "mapsmd03.mix"
    "movmd03.mix"
    "multimd.mix"
    "thememd.mix"
    "gamemd.exe"
    "RA2MD.INI"
    "Blowfish.dll"
    "BINKW32.DLL"
  )
endif()

message(STATUS "Configuring ${APP_NAME} (${TARGET_NAME}) with bundle ID ${BUNDLE_ID}")

# Enable Xcode scheme generation
set(CMAKE_XCODE_GENERATE_SCHEME ON)
set(CMAKE_XCODE_SCHEME_DISABLE_MAIN_THREAD_CHECKER ON)

# iOS Dependencies (Both Device and Simulator are supported from the same project)
set(IOS_DEV_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/deps/installed-ios")
set(IOS_SIM_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/deps/installed-ios-sim")

# Recompiled C sources
file(GLOB GEN_SOURCES CONFIGURE_DEPENDS "${GEN_DIR}/*.c")
if(NOT GEN_SOURCES)
  message(FATAL_ERROR "No generated sources found in ${GEN_DIR}. Run setup.py first.")
endif()

# Win32 HLE runtime sources
file(GLOB HLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/win32hle/*.c")
list(FILTER HLE_SOURCES EXCLUDE REGEX "_selftest\\.c$")

# Runtime sources
set(RUNTIME_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/main.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/blowfish.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/script.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/mods.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/menu.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/hdvox.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/queuefive.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/ios/ios_display_bridge.m"
  ${HLE_SOURCES}
)

# App bundle target
add_executable(${TARGET_NAME} MACOSX_BUNDLE ${RUNTIME_SOURCES} ${GEN_SOURCES})

# Target properties
set_target_properties(${TARGET_NAME} PROPERTIES
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_INFO_PLIST "${INFO_PLIST}"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${BUNDLE_ID}"
  MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUNDLE_ID}"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
  XCODE_ATTRIBUTE_ARCHS "arm64"
  XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "${APP_ICON_NAME}"
  "XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS[sdk=iphoneos*]" "$(inherited) ${IOS_DEV_DEPS}/lib"
  "XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS[sdk=iphonesimulator*]" "$(inherited) ${IOS_SIM_DEPS}/lib"
)

set_target_properties(${TARGET_NAME} PROPERTIES
  XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
)
if(DEVELOPMENT_TEAM)
  set_target_properties(${TARGET_NAME} PROPERTIES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
  )
endif()

# Include App Icon Assets
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/ios/Assets.xcassets")
  target_sources(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/ios/Assets.xcassets")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/packaging/ios/Assets.xcassets" PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
  )
endif()

# Asset injection into App Bundle (always resolve exact disk casing)
file(GLOB GAME_FILES_ON_DISK CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ra2-files/*")

function(add_game_asset target_name asset_name)
  string(TOLOWER "${asset_name}" asset_lower)
  set(found_asset "")
  foreach(cand IN LISTS GAME_FILES_ON_DISK)
    get_filename_component(cand_name "${cand}" NAME)
    string(TOLOWER "${cand_name}" cand_lower)
    if(cand_lower STREQUAL asset_lower)
      set(found_asset "${cand}")
      break()
    endif()
  endforeach()
  if(found_asset)
    target_sources(${target_name} PRIVATE "${found_asset}")
    set_source_files_properties("${found_asset}" PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
    )
  else()
    message(WARNING "Asset ${asset_name} not found in ra2-files/")
  endif()
endfunction()

foreach(asset IN LISTS TARGET_ASSETS)
  add_game_asset(${TARGET_NAME} "${asset}")
endforeach()

# Skirmish map packs (.mmx for RA2, .yro and .mmx for YR)
file(GLOB RA2_MAPS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ra2-files/*.mmx")
file(GLOB YR_MAPS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ra2-files/*.yro")
if(RA2_TARGET STREQUAL "game")
  set(TARGET_MAPS ${RA2_MAPS})
else()
  set(TARGET_MAPS ${RA2_MAPS} ${YR_MAPS})
endif()
foreach(map_file IN LISTS TARGET_MAPS)
  target_sources(${TARGET_NAME} PRIVATE "${map_file}")
  set_source_files_properties("${map_file}" PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
  )
endforeach()

# Folders (Taunts, RMCache)
set(COMMON_ASSET_DIRS "Taunts" "RMCache")
foreach(dir_name IN LISTS COMMON_ASSET_DIRS)
  string(TOLOWER "${dir_name}" dir_lower)
  set(found_dir "")
  foreach(cand IN LISTS GAME_FILES_ON_DISK)
    if(IS_DIRECTORY "${cand}")
      get_filename_component(cand_name "${cand}" NAME)
      string(TOLOWER "${cand_name}" cand_lower)
      if(cand_lower STREQUAL dir_lower)
        set(found_dir "${cand}")
        break()
      endif()
    endif()
  endforeach()
  if(found_dir)
    target_sources(${TARGET_NAME} PRIVATE "${found_dir}")
    set_source_files_properties("${found_dir}" PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
    )
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${found_dir}"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>/${dir_name}"
    )
  endif()
endforeach()

# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/ios"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/win32hle"
  "${GEN_DIR}"
  "${IOS_DEV_DEPS}/include"
  "${IOS_DEV_DEPS}/include/SDL2"
)

# Compiler definitions
if(RA2_TARGET STREQUAL "game")
  target_compile_definitions(${TARGET_NAME} PRIVATE
    RA2_TARGET_GAME=1
    RECOMP_RETADDR=0u
    RECOMP_LOCAL_REGS
    HLE_WITH_FFMPEG
  )
else()
  target_compile_definitions(${TARGET_NAME} PRIVATE
    RA2_TARGET_GAMEMD=1
    RECOMP_RETADDR=0u
    RECOMP_LOCAL_REGS
    HLE_WITH_FFMPEG
  )
endif()

# Link libraries (conditional search paths link appropriate device/sim slices)
target_link_libraries(${TARGET_NAME} PRIVATE
  SDL2main
  SDL2
  SDL2_ttf
  freetype
  avformat
  avcodec
  swresample
  avutil
  "-framework Foundation"
  "-framework UIKit"
  "-framework Metal"
  "-framework MetalKit"
  "-framework AudioToolbox"
  "-framework AVFoundation"
  "-framework CoreGraphics"
  "-framework QuartzCore"
  "-framework CoreAudio"
  "-framework CoreVideo"
  "-framework CoreBluetooth"
  "-framework CoreMotion"
  "-framework CoreHaptics"
  "-framework GameController"
  "-framework OpenGLES"
  pthread m z bz2 iconv
)

# Compiler options
set(GEN_OPT "-O1" CACHE STRING "Optimization for the generated C code")
set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "-w;${GEN_OPT};-fno-strict-aliasing;-fwrapv")
set_source_files_properties(${RUNTIME_SOURCES} PROPERTIES COMPILE_OPTIONS "-O3")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/platform/ios/ios_display_bridge.m" PROPERTIES COMPILE_OPTIONS "-x;objective-c;-O3")
target_compile_options(${TARGET_NAME} PRIVATE -fno-strict-aliasing -fwrapv)
