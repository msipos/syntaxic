#   USAGE
# mkdir build && cd build
#
#   LINUX
# cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/home/max/Qt/5.4/gcc_64/lib/cmake ..
#
#   WINDOWS
# cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/5.4/msvc2013/lib/cmake ..
# C:/Program\ Files\ \(x86\)/MSBuild/12.0/bin/MSBuild.exe utests.vcxproj //p:Configuration=Release
# C:/Program\ Files\ \(x86\)/MSBuild/12.0/bin/MSBuild.exe syntaxic.vcxproj //p:Configuration=Release
#
#   MAC
# cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/Users/max/Qt5.4.0/5.4/clang_64/lib/cmake

project(Syntaxic)
cmake_minimum_required(VERSION 2.8.11)

# Useful prerequisites:

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS TRUE)
  add_definitions(-DCMAKE_WINDOWS)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX TRUE)
  add_definitions(-DCMAKE_LINUX)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOSX TRUE)
  add_definitions(-DCMAKE_MACOSX)
endif()

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(DEBUG_BUILD TRUE)
  add_definitions(-DCMAKE_DEBUG)
else()
  set(RELEASE_BUILD TRUE)
  add_definitions(-DCMAKE_RELEASE)
endif()

#######################################################################
# Find libraries

###### JSON
add_definitions(-DJSON_USE_EXCEPTION)

###### QT

# Explicitly set MOC:
set(CMAKE_AUTOMOC ON)
# As moc and Ui files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# This unfortunately only works with CMake 3.+ (?)
#set(CMAKE_AUTOUIC ON)

###### Qt5Core

find_package(Qt5Core REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})
add_definitions(${Qt5Core_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")
list(APPEND tde_libs Qt5::Core)
list(APPEND utests_libs Qt5::Core)

###### Qt5Gui
find_package(Qt5Gui REQUIRED)
include_directories(${Qt5Gui_INCLUDE_DIRS})
add_definitions(${Qt5Gui_DEFINITIONS})
list(APPEND tde_libs Qt5::Gui)
list(APPEND utests_libs Qt5::Gui)

###### Qt5Widgets
find_package(Qt5Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})
list(APPEND tde_libs Qt5::Widgets)

###### Qt5Network
find_package(Qt5Network REQUIRED)
include_directories(${Qt5Network_INCLUDE_DIRS})
add_definitions(${Qt5Network_DEFINITIONS})
list(APPEND tde_libs Qt5::Network)

###### Re2

find_library(library_re2 re2 third-party/lib NO_DEFAULT_PATH)
list(APPEND tde_libs ${library_re2})
list(APPEND utests_libs ${library_re2})

message(STATUS " * * * Required libraries for TDE: ${tde_libs}")
message(STATUS " * * * Required libraries for utests: ${utests_libs}")

#######################################################################
# CXX FLAGS:

if (LINUX)
  # Warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wfatal-errors -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdisabled-optimization -Wno-unused")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-null-sentinel -Winvalid-pch")

  # Enable C++11:
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  # Set RPATH to $ORIGIN, special value that refers to dir of executable, this will only be applied
  # in the make install step.
  set(CMAKE_INSTALL_RPATH "\$ORIGIN")

  # On Linux, compile in standard libs statically, libstdc++ is an ABI nightmare
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

elseif(MACOSX)
  # Warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wfatal-errors -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdisabled-optimization -Wno-unused")

  # Enable C++11:
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  # Apple ships an old GNU c++ lib, tell it to  use Clang's instead.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

#######################################################################
# LM:
add_definitions(-DLMCHECK0)
add_definitions(-DLMCHECK1)
add_definitions(-DLMCHECK2)

#######################################################################
# Build

# Include paths:
include_directories("src")
include_directories("third-party/include")

set(common_sources
        src/core/cont_file.cpp
        src/core/encoding.cpp
        src/core/flow_grid.cpp
        src/core/line.cpp
        src/core/mapper.cpp
        src/core/rich_text.cpp
        src/core/text_buffer.cpp
        src/core/text_edit.cpp
        src/core/text_file.cpp
        src/core/undo_manager.cpp
        src/core/util.cpp
        src/core/utf8_util.cpp
        src/core/util_glob.cpp
        src/core/util_path.cpp
        src/core/text_view.cpp
        src/core/word_def.cpp
        src/choices.cpp
        src/console.cpp
        src/doc.cpp
        src/document.cpp
        src/file_browser.cpp
        src/file_io_provider.cpp
        src/js_console.cpp
        src/keymapper.cpp
        src/known_documents.cpp
        src/lm.cpp
        src/master.cpp
        src/master_navigators.cpp
        src/master_io_provider.cpp
        src/master_js.cpp
        src/my_process.cpp
        src/preferences.cpp
        src/process.cpp
        src/process_impl.cpp
        src/project.cpp
        src/recents.cpp
        src/settings.cpp
        src/ssh_io_provider.cpp
        src/statlang/statlang.cpp
        src/statlang/symboldb.cpp
        src/statlang/tokenizer.cpp
        #src/statlang/indep/sl.cpp
        src/theme.cpp
        src/tool.cpp
        third-party/src/duktape.c
        third-party/src/jsoncpp.cpp)

if (LINUX)
  add_executable(lmgen src/lmgenmain.cpp src/core/utf8_util.cpp src/lmgen.cpp src/lm.cpp)
endif()

# QT UI (not used for now):

#set(qt_uis
#      src/qtgui/main_window.ui
#   )
#qt5_wrap_ui(qt_uis_srcs ${qt_uis})
#message(STATUS " * * * qt_uis_srcs = ${qt_uis_srcs}")

# QT Resources:

qt5_add_resources(qt_resources resources.qrc)

set(qt_sources
      ${qt_uis_srcs}
      ${qt_resources}
      src/qtgui/complete_dialog.cpp
      src/qtgui/dock.cpp
      src/qtgui/docs_gui.cpp
      src/qtgui/editor.cpp
      src/qtgui/feedback_box.cpp
      src/qtgui/glyph_store.cpp
      src/qtgui/invoke_dialog.cpp
      src/qtgui/jf_dialog.cpp
      src/qtgui/main_window.cpp
      src/qtgui/menu_manager.cpp
      src/qtgui/my_line_edit.cpp
      src/qtgui/my_tab_bar.cpp
      src/qtgui/pref_dialog.cpp
      src/qtgui/project_dialog.cpp
      src/qtgui/qtheme.cpp
      src/qtgui/register_dialog.cpp
      src/qtgui/sar_dialog.cpp
      src/qtgui/sidebar_tree.cpp
      src/qtgui/ssh_gui.cpp
   )

if (LINUX)
  add_executable(slindep EXCLUDE_FROM_ALL
      src/statlang/indep/slmain.cpp
      ${qt_sources} ${common_sources}
      )
  target_link_libraries(slindep ${tde_libs})
endif()

# Utests
add_executable(utests src/unit_tests.cpp src/lmgen.cpp src/qtgui/qtmain.cpp ${qt_sources} ${common_sources})
target_compile_definitions(utests PUBLIC NO_QT_MAIN)
target_link_libraries(utests ${tde_libs})

list(APPEND qt_sources src/qtgui/qtmain.cpp)

# Installation
set(CMAKE_INSTALL_PREFIX prefix)
set(syntaxic syntaxic)
if (WINDOWS)
  add_executable(${syntaxic} WIN32 ${common_sources} ${qt_sources} resources/syntaxic.rc)
elseif(MACOSX)
  set(syntaxic Syntaxic)
  add_executable(Syntaxic MACOSX_BUNDLE ${common_sources} ${qt_sources} resources/icon.icns)

  # Copy binary
  install(TARGETS Syntaxic BUNDLE DESTINATION .)

  # Icon
  # This goes into plist
  set(MACOSX_BUNDLE_ICON_FILE icon.icns)

  set_source_files_properties(resources/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

  # Seems to only work when absolute path is given to the bundle.
  set(syntaxic_bundle_path "/Users/max/repos/syntaxic/build/Syntaxic.app")
  set(qt5_dir /Users/max/Qt5.4.0/5.4/clang_64)

  # Must install stuff first before running fixup. I don't know why I need to ship this...
  install(FILES
    /usr/lib/libz.1.dylib
    DESTINATION ${syntaxic_bundle_path}/Contents/MacOS
  )
  install(FILES
    /usr/lib/libz.1.2.5.dylib
    DESTINATION ${syntaxic_bundle_path}/Contents/MacOS
  )

  # Must install stuff first before running fixup. This is a plugin
  install(FILES
    ${qt5_dir}/plugins/platforms/libqcocoa.dylib
    DESTINATION ${syntaxic_bundle_path}/Contents/MacOS/platforms
  )

  # Install Info.plist file
  install(FILES
    mac_pkg/Info.plist DESTINATION ${syntaxic_bundle_path}/Contents)

  # Install meta stuff
  install(DIRECTORY meta/ DESTINATION ${syntaxic_bundle_path}/Contents/MacOS)
  install(FILES  bin/syntaxic_local_wrapper.mac DESTINATION ${syntaxic_bundle_path}/Contents/MacOS RENAME syntaxic_local_wrapper PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

  # Install license and help
  install(FILES
    SYNTAXIC_LICENSE DESTINATION ${syntaxic_bundle_path}/Contents/MacOS)
  install(FILES
    SYNTAXIC_HELP DESTINATION ${syntaxic_bundle_path}/Contents/MacOS)
  install(FILES
    SYNTAXIC_WELCOME DESTINATION ${syntaxic_bundle_path}/Contents/MacOS)

  # Specify the libqcocoa for extra fixup
  set(extra_fixup_items ${syntaxic_bundle_path}/Contents/MacOS/platforms/libqcocoa.dylib)

  install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"${syntaxic_bundle_path}\"
                 \"${extra_fixup_items}\"
                 \"${CMAKE_LIBRARY_PATH}\")
    verify_app(Syntaxic.app)
    verify_bundle_prerequisites(Syntaxic.app RV IV)
    verify_bundle_symlinks(Syntaxic.app RV IV)
  ")

  set(CPACK_BINARY_DRAGNDROP ON)
  include(CPack)

else()
  add_executable(syntaxic ${common_sources} ${qt_sources})
  install(TARGETS syntaxic RUNTIME DESTINATION .)
  install(TARGETS lmgen RUNTIME DESTINATION .)
endif()
target_link_libraries(${syntaxic} ${tde_libs})