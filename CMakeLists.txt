cmake_minimum_required(VERSION 3.20)

project(ALAC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(alac
        codec/ag_dec.c
        codec/ag_enc.c
        codec/aglib.h
        codec/ALACBitUtilities.c
        codec/ALACDecoder.cpp
        codec/ALACEncoder.cpp
        codec/dp_dec.c
        codec/dp_enc.c
        codec/dplib.h
        codec/EndianPortable.h
        codec/matrix_dec.c
        codec/matrix_enc.c
        codec/matrixlib.h
        include/ALACAudioTypes.h
        include/ALACBitUtilities.h
        include/ALACDecoder.h
        include/ALACEncoder.h
        include/ALACLib.h
)

target_include_directories(alac PUBLIC include)

get_target_property(ALAC_LIB_TYPE alac TYPE)
if (ALAC_LIB_TYPE STREQUAL "SHARED_LIBRARY")
    target_compile_definitions(alac PRIVATE ALAC_DYNAMIC)
    target_compile_definitions(alac PUBLIC ALAC_DYNAMIC_IMPORT)
endif ()
