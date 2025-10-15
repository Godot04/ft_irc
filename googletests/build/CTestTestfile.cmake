# CMake generated Testfile for 
# Source directory: /home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests
# Build directory: /home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(CommandTest "/home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests/build/command_test")
set_tests_properties(CommandTest PROPERTIES  _BACKTRACE_TRIPLES "/home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests/CMakeLists.txt;49;add_test;/home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests/CMakeLists.txt;0;")
add_test(ChannelsClientsManagerTest "/home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests/build/channels_clients_manager_test")
set_tests_properties(ChannelsClientsManagerTest PROPERTIES  _BACKTRACE_TRIPLES "/home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests/CMakeLists.txt;50;add_test;/home/zkhojazo/Desktop/ft_irc_dir/ft_irc_github/googletests/CMakeLists.txt;0;")
subdirs("googletest")
