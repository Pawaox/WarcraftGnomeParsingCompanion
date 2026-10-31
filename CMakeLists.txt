cmake_minimum_required(VERSION 3.24)

project(GnomeparsingCompanion VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
include(cmake/EmbedResources.cmake)

set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxBUILD_USE_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)

foreach(feature WEBVIEW MEDIACTRL STC RICHTEXT AUI PROPGRID RIBBON XRC HTML WXHTML_HELP HELP MS_HTML_HELP OPENGL XML)
    set(wxUSE_${feature} OFF CACHE BOOL "" FORCE)
endforeach()

foreach(library LIBJPEG LIBTIFF LIBWEBP)
    set(wxUSE_${library} OFF CACHE STRING "" FORCE)
endforeach()

FetchContent_Declare(wxWidgets
    URL https://github.com/wxWidgets/wxWidgets/releases/download/v3.3.4/wxWidgets-3.3.4.zip
    URL_HASH SHA256=a604fe40a59e6515eac3efe58deca38b54b7ef7f3c1df0906559bde6de1e2f58
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(wxWidgets)

add_executable(GnomeparsingCompanion WIN32 MACOSX_BUNDLE
    src/App.cpp
    src/Core/AppPaths.cpp
    src/Core/Expansion.cpp
    src/Core/LogArchive.cpp
    src/Core/Settings.cpp
    src/Core/Startup.cpp
    src/Core/WowFolders.cpp
    src/Ui/FolderPromptView.cpp
    src/Ui/Images.cpp
    src/Ui/LogsView.cpp
    src/Ui/MainFrame.cpp
    src/Ui/NavBar.cpp
    src/Ui/SettingsView.cpp
    src/Ui/TrayIcon.cpp
)

if(WIN32)
    target_sources(GnomeparsingCompanion PRIVATE resources/app.rc)
    target_link_options(GnomeparsingCompanion PRIVATE /MANIFEST:NO)
endif()

target_include_directories(GnomeparsingCompanion PRIVATE src)
target_compile_definitions(GnomeparsingCompanion PRIVATE GP_VERSION="${PROJECT_VERSION}")
target_link_libraries(GnomeparsingCompanion PRIVATE wx::core wx::base)

embed_resources(GnomeparsingCompanion
    resources/logo.png
    resources/vanilla.png
    resources/tbc.png
    resources/wotlk.png
    resources/forever.png
)
