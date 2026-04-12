cmake_minimum_required(VERSION 3.16)
project(kwin-genie-open)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_DEFAULT_MAJOR_VERSION 6)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(kwin_genie_open MODULE 
    genieopen.cpp
    plugin.cpp
)

target_include_directories(kwin_genie_open PRIVATE
    /usr/include/kwin
    /usr/include/kwin/effect
    /usr/include/kwin/scene
    /usr/include/KF6/KCoreAddons
    /usr/include/KF6/KConfig
    /usr/include/KF6/KConfigCore
    /usr/include/KF6/KConfigGui
)

target_link_libraries(kwin_genie_open
    Qt6::Core
    Qt6::Gui
    /usr/lib/libkwin.so
    /usr/lib/libKF6CoreAddons.so
    /usr/lib/libKF6ConfigCore.so
)

install(TARGETS kwin_genie_open DESTINATION /usr/lib/qt6/plugins/kwin/effects/plugins)
install(FILES metadata.json DESTINATION /usr/share/kwin/effects/kwin_genie_open)

#######################################
# Config KCM
find_package(Qt6 REQUIRED COMPONENTS DBus Widgets)
find_package(KF6KCMUtils REQUIRED)
find_package(KF6CoreAddons REQUIRED)
find_package(KF6I18n REQUIRED)
find_package(KF6Config REQUIRED)

kconfig_add_kcfg_files(genieopen_config_SRCS genieopenconfig.kcfgc)
qt6_wrap_ui(genieopen_config_SRCS genieopen_config.ui)

add_library(kwin_genieopen_config MODULE
    genieopen_config.cpp
    genieopen_config.json
    ${genieopen_config_SRCS}
)

target_include_directories(kwin_genieopen_config PRIVATE
    /usr/include/kwin
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(kwin_genieopen_config
    Qt6::Core
    Qt6::Gui
    Qt6::DBus
    Qt6::Widgets
    KF6::KCMUtils
    KF6::CoreAddons
    KF6::I18n
    KF6::ConfigCore
    KF6::ConfigGui
)

install(TARGETS kwin_genieopen_config DESTINATION /usr/lib/qt6/plugins/kwin/effects/configs)