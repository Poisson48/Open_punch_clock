find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
pkg_check_modules(SECP256K1 REQUIRED IMPORTED_TARGET libsecp256k1)

add_library(net STATIC
    net/nostr.cpp
    net/relayclient.cpp
    net/relaypool.cpp
    net/pushclient.cpp
)
target_include_directories(net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(net PUBLIC Qt6::Core Qt6::Network Qt6::WebSockets)

add_library(core STATIC
    core/pairing.cpp
    core/csv.cpp
    core/zip.cpp
    core/timecalculator.cpp
    core/xlsx.cpp
)
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)
target_link_libraries(core PUBLIC Qt6::Core)

add_library(cryptolayer STATIC net/crypto.cpp)
target_include_directories(cryptolayer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cryptolayer PUBLIC Qt6::Core net PkgConfig::SODIUM PkgConfig::SECP256K1)

add_library(store STATIC store/database.cpp)
target_include_directories(store PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(store PUBLIC Qt6::Core Qt6::Sql core)

set(APPLIB_SOURCES
    app/appcontroller.cpp
    app/punchengine.cpp
    app/projectmodel.cpp
    app/timeentrymodel.cpp
    app/syncengine.cpp
    app/platform.cpp
    app/theme.cpp
    app/updater.cpp
    app/permissions.cpp
)

add_library(applib STATIC ${APPLIB_SOURCES})
target_include_directories(applib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(applib PUBLIC
    core cryptolayer net store
    Qt6::Core Qt6::Concurrent Qt6::Network Qt6::Quick Qt6::Sql Qt6::WebSockets
    PkgConfig::SODIUM PkgConfig::SECP256K1
)

if(PUNCH_HAS_GPS)
    target_link_libraries(applib PUBLIC Qt6::Positioning)
    target_compile_definitions(applib PUBLIC PUNCH_HAS_GPS)
endif()

if(TARGET Qt6::Widgets)
    target_link_libraries(applib PUBLIC Qt6::Widgets)
endif()

qt_add_executable(openpunchclock app/main.cpp)

set(PUNCH_QML_FILES
    qml/Main.qml
    qml/PunchPage.qml
    qml/ProjectsPage.qml
    qml/HistoryPage.qml
    qml/TimeCardEditor.qml
    qml/SettingsPage.qml
    qml/ReportsPage.qml
    qml/TimeUtils.qml
    qml/ColoDialog.qml
    qml/ColoMenu.qml
    qml/ColoTextField.qml
    qml/Icon.qml
    qml/ChangelogDialog.qml
    qml/FilePickers.qml
    qml/ServingsStepper.qml
)

set_source_files_properties(qml/TimeUtils.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(openpunchclock
    URI OpenPunchClock
    VERSION 1.0
    QML_FILES ${PUNCH_QML_FILES}
)

target_link_libraries(openpunchclock PRIVATE
    applib cryptolayer net store core
    Qt6::Core Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::WebSockets
    PkgConfig::SODIUM PkgConfig::SECP256K1
)

set_property(TARGET openpunchclock PROPERTY
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android")
set_property(TARGET openpunchclock PROPERTY QT_ANDROID_TARGET_SDK_VERSION 34)

set(PUNCH_VERSION_NAME "0.1.0" CACHE STRING "android:versionName")
set(PUNCH_VERSION_CODE "1" CACHE STRING "android:versionCode")
set_property(TARGET openpunchclock PROPERTY QT_ANDROID_VERSION_NAME "${PUNCH_VERSION_NAME}")
set_property(TARGET openpunchclock PROPERTY QT_ANDROID_VERSION_CODE "${PUNCH_VERSION_CODE}")
target_compile_definitions(applib PUBLIC PUNCH_APP_VERSION="${PUNCH_VERSION_NAME}")

if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(android_openssl
        GIT_REPOSITORY https://github.com/KDAB/android_openssl.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(openpunchclock)
endif()
