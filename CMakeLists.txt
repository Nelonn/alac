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
        codec/ALACBitUtilities.h
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
        include/ALACDecoder.h
        include/ALACEncoder.h
)

target_include_directories(alac PUBLIC include)
