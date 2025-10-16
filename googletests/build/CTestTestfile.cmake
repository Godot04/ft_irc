# CMake generated Testfile for 
# Source directory: /home/opopov/Documents/ft_irc/googletests
# Build directory: /home/opopov/Documents/ft_irc/googletests/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(CommandTest "/home/opopov/Documents/ft_irc/googletests/build/command_test")
set_tests_properties(CommandTest PROPERTIES  _BACKTRACE_TRIPLES "/home/opopov/Documents/ft_irc/googletests/CMakeLists.txt;49;add_test;/home/opopov/Documents/ft_irc/googletests/CMakeLists.txt;0;")
add_test(ChannelsClientsManagerTest "/home/opopov/Documents/ft_irc/googletests/build/channels_clients_manager_test")
set_tests_properties(ChannelsClientsManagerTest PROPERTIES  _BACKTRACE_TRIPLES "/home/opopov/Documents/ft_irc/googletests/CMakeLists.txt;50;add_test;/home/opopov/Documents/ft_irc/googletests/CMakeLists.txt;0;")
subdirs("googletest")
