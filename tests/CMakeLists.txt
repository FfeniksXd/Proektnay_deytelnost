# Пока делаем один тест-заглушку для проверки GTest
add_executable(dummy_test dummy_test.cpp)
target_link_libraries(dummy_test PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(dummy_test)