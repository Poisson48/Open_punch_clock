cmake_minimum_required(VERSION 3.24)
project(openpunchclock_tests)

qt_add_executable(tst_smoke tst_smoke.cpp)
target_link_libraries(tst_smoke PRIVATE Qt6::Test)
add_test(NAME smoke COMMAND tst_smoke)

qt_add_executable(tst_database tst_database.cpp)
target_link_libraries(tst_database PRIVATE store core Qt6::Test Qt6::Sql)
add_test(NAME database COMMAND tst_database)

qt_add_executable(tst_timecalc tst_timecalc.cpp)
target_include_directories(tst_timecalc PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_timecalc PRIVATE core Qt6::Test)
add_test(NAME timecalc COMMAND tst_timecalc)

qt_add_executable(tst_dataexchange tst_dataexchange.cpp)
target_include_directories(tst_dataexchange PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_dataexchange PRIVATE core Qt6::Test)
add_test(NAME dataexchange COMMAND tst_dataexchange)

qt_add_executable(tst_punch tst_punch.cpp)
target_include_directories(tst_punch PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_punch PRIVATE applib store core Qt6::Test Qt6::Sql)
add_test(NAME punch COMMAND tst_punch)
set_tests_properties(punch PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(tst_qml tst_qml.cpp)
target_include_directories(tst_qml PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_qml PRIVATE applib store core Qt6::Test)
add_test(NAME qml COMMAND tst_qml)
set_tests_properties(qml PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(tst_i18n tst_i18n.cpp)
target_include_directories(tst_i18n PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_i18n PRIVATE applib Qt6::Test)
add_test(NAME i18n COMMAND tst_i18n)
set_tests_properties(i18n PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
