cmake_minimum_required(VERSION 3.19)
project(spoLab15 C)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

set(CMAKE_C_STANDARD 99)

add_executable(server utils/const.h server/server.c server/server.h server/server_main.c utils/message.c utils/message.h utils/linked_list.c utils/linked_list.h)
add_executable(client client/client.c client/client.h utils/message.c utils/message.h client/client_main.c)
target_link_libraries(server ${LIBXML2_LIBRARIES})