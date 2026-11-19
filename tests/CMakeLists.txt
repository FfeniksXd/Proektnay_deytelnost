# Пока делаем один тест-заглушку для проверки GTest
add_executable(dummy_test dummy_test.cpp)
target_link_libraries(dummy_test PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(dummy_test)

#  тесты для сессии
add_executable(session_tests test_session.cpp)
target_link_libraries(session_tests PRIVATE session core GTest::gtest_main)
gtest_discover_tests(session_tests)

add_executable(codec_tests test_codec.cpp)
target_link_libraries(codec_tests PRIVATE session core GTest::gtest_main)
gtest_discover_tests(codec_tests)

add_executable(async_tests test_async.cpp)
target_link_libraries(async_tests PRIVATE session core GTest::gtest_main)
gtest_discover_tests(async_tests)

add_executable(network_host_tests test_network_host.cpp)
target_link_libraries(network_host_tests PRIVATE session core GTest::gtest_main)
gtest_discover_tests(network_host_tests)