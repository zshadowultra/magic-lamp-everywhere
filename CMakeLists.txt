cmake_minimum_required(VERSION 3.16)
project(kwin-genie-open)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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