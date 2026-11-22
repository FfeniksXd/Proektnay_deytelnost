# Пока делаем один тест-заглушку для проверки GTest
add_executable(dummy_test dummy_test.cpp)
target_link_libraries(dummy_test PRIVATE GTest::gtest_main)

add_executable(core_tests core_Tests.cpp)
target_link_libraries(core_tests PRIVATE core gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(dummy_test)
gtest_discover_tests(core_tests) 