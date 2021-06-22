cmake_minimum_required(VERSION 3.19)
project(spoLab15 C)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

set(CMAKE_C_STANDARD 99)

add_executable(server utils/const.h server/server.c server/server.h server/server_main.c utils/message.c utils/message.h utils/linked_list.c utils/linked_list.h server/datafile/datafile.c server/datafile/datafile.h server/datafile/cells.h server/datafile/packs.h server/datafile/const.h server/datafile/blocks.c server/datafile/blocks.h)
add_executable(client client/client.c client/client.h utils/message.c utils/message.h client/client_main.c)
add_executable(datafile server/datafile/datafile.c server/datafile/datafile.h server/datafile/datafile_main.c server/datafile/cells.h)
target_link_libraries(server ${LIBXML2_LIBRARIES})