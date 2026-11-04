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
FetchContent_Declare(nlohmann_json_header
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    URL_HASH SHA256=9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/json/nlohmann
)
FetchContent_MakeAvailable(wxWidgets nlohmann_json_header)

add_executable(GnomeparsingCompanion WIN32 MACOSX_BUNDLE
    src/App.cpp
    src/Core/Api.cpp
    src/Core/AppPaths.cpp
    src/Core/Checksum.cpp
    src/Core/CombatLogs.cpp
    src/Core/Expansion.cpp
    src/Core/LogArchive.cpp
    src/Core/LogScanner.cpp
    src/Core/LogUpload.cpp
    src/Core/Settings.cpp
    src/Core/Startup.cpp
    src/Core/TokenStore.cpp
    src/Core/WowFolders.cpp
    src/Ui/Controls/Button.cpp
    src/Ui/Controls/Dropdown.cpp
    src/Ui/Controls/ImageTab.cpp
    src/Ui/Controls/SelectRow.cpp
    src/Ui/Controls/Separator.cpp
    src/Ui/Controls/Tab.cpp
    src/Ui/Controls/TextTab.cpp
    src/Ui/Controls/ToggleSwitch.cpp
    src/Ui/FolderPromptView.cpp
    src/Ui/Fonts.cpp
    src/Ui/Images.cpp
    src/Ui/LoginView.cpp
    src/Ui/LogPickerDialog.cpp
    src/Ui/LogsView.cpp
    src/Ui/MainFrame.cpp
    src/Ui/NavBar.cpp
    src/Ui/Palette.cpp
    src/Ui/SettingsView.cpp
    src/Ui/TrayIcon.cpp
    src/Ui/UploadView.cpp
)

if(WIN32)
    target_sources(GnomeparsingCompanion PRIVATE resources/app.rc)
    target_link_options(GnomeparsingCompanion PRIVATE /MANIFEST:NO)
    target_link_libraries(GnomeparsingCompanion PRIVATE crypt32)
endif()

target_include_directories(GnomeparsingCompanion PRIVATE src ${CMAKE_BINARY_DIR}/_deps/json)
target_compile_definitions(GnomeparsingCompanion PRIVATE GP_VERSION="${PROJECT_VERSION}")
target_link_libraries(GnomeparsingCompanion PRIVATE wx::core wx::net wx::base)

embed_resources(GnomeparsingCompanion
    resources/logo.png
    resources/vanilla.png
    resources/tbc.png
    resources/wotlk.png
    resources/forever.png
    resources/fonts/NunitoSans-Regular.ttf
    resources/fonts/NunitoSans-Italic.ttf
    resources/fonts/NunitoSans-SemiBold.ttf
    resources/fonts/NunitoSans-Bold.ttf
)
