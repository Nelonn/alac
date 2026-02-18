cmake_minimum_required(VERSION 3.20)

project(ALAC VERSION 1.0.0 LANGUAGES C CXX)

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

target_include_directories(alac PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(alac PRIVATE ALAC_DYNAMIC)
    target_compile_definitions(alac INTERFACE ALAC_DYNAMIC_IMPORT)
endif ()

set_target_properties(alac PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        EXPORT_NAME alac
)

add_library(alac::alac ALIAS alac)

include(GNUInstallDirs)

install(TARGETS alac
        EXPORT alacTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)

install(EXPORT alacTargets
        FILE alacTargets.cmake
        NAMESPACE alac::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/alac
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/alacConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/alacConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/alac
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/alacConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/alacConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/alacConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/alac
)
