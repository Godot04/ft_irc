cmake_minimum_required(VERSION 3.10)
project(ft_irc_serv)

set(CMAKE_CXX_STANDARD 98)

# Add GoogleTest (as submodule)
add_subdirectory(googletest)

enable_testing()

# Include project source directories
include_directories(${CMAKE_SOURCE_DIR}/../inc)

# Source files to compile
set(COMMON_SOURCES
    ${CMAKE_SOURCE_DIR}/../srcs/IRCCommand.cpp

)

set(CHANNELS_CLIENTS_MANAGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../srcs/ChannelsClientsManager.cpp
    ${CMAKE_SOURCE_DIR}/../srcs/Client.cpp
    ${CMAKE_SOURCE_DIR}/../srcs/Channel.cpp
    ${CMAKE_SOURCE_DIR}/../srcs/Reply.cpp
    # Add any other needed source files
)

# # Add your test executables
# add_executable(hello_world_test tests/hello_world_test.cpp)
# add_executable(command_test tests/test_comand_class.cpp ${COMMON_SOURCES})

# # Link GoogleTest libraries
# target_link_libraries(hello_world_test gtest gtest_main pthread)
# target_link_libraries(command_test gtest gtest_main pthread)

# # Register the tests with CTest
# add_test(NAME HelloWorldTest COMMAND hello_world_test)
# add_test(NAME CommandTest COMMAND command_test)


# ...existing code...

add_executable(command_test tests/test_comand_class.cpp ${COMMON_SOURCES})
add_executable(channels_clients_manager_test tests/test_channels_clients_manager.cpp ${COMMON_SOURCES} ${CHANNELS_CLIENTS_MANAGER_SOURCES})

target_link_libraries(command_test gtest gtest_main pthread)
target_link_libraries(channels_clients_manager_test gtest gtest_main pthread)

add_test(NAME CommandTest COMMAND command_test)
add_test(NAME ChannelsClientsManagerTest COMMAND channels_clients_manager_test)

# Custom targets for convenience
add_custom_target(irc_commands
    COMMAND ${CMAKE_COMMAND} --build . --target command_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(channels_clients_manager
    COMMAND ${CMAKE_COMMAND} --build . --target channels_clients_manager_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)







# cmake_minimum_required(VERSION 3.10)
# project(ft_irc_serv)

# set(CMAKE_CXX_STANDARD 98)

# # Add GoogleTest (as submodule)
# add_subdirectory(googletest)

# enable_testing()

# # Add your test executable
# add_executable(hello_world_test tests/hello_world_test.cpp)

# # Link GoogleTest libraries
# target_link_libraries(hello_world_test gtest gtest_main pthread)

# # Register the test with CTest
# add_test(NAME HelloWorldTest COMMAND hello_world_test)