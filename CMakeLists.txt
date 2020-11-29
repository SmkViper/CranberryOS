cmake_minimum_required(VERSION 3.18)

project(CranberryOS VERSION 1.0
                    DESCRIPTION "Silly little toy OS for a Raspberry Pi"
                    LANGUAGES C CXX ASM)

add_subdirectory(kernel)
