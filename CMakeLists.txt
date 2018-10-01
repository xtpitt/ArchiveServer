cmake_minimum_required(VERSION 3.12)
project(ArchiveServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(ArchiveServer main.cpp tcpconnection.cpp tcpconnection.h)