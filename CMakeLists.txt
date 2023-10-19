cmake_minimum_required(VERSION 3.5)
project(YarnDesktopClient) 
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk4)

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)
pkg_check_modules(LIBSECRET REQUIRED libsecret-1)

include_directories(${GTK_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${LIBSECRET_INCLUDE_DIRS})
set(SOURCES YarnDesktopClient.cpp YarnDesktopClient.h statuspost.h)
add_executable(${PROJECT_NAME} ${SOURCES}) 
target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES} ${CURL_LIBRARIES} ${LIBSECRET_LIBRARIES} stdc++fs)
install(TARGETS YarnDesktopClient DESTINATION bin)
