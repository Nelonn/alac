#pragma once

#if defined(_WIN32)
#define ALAC_DLL_EXPORT __declspec(dllexport)
#define ALAC_DLL_IMPORT __declspec(dllimport)
#else
#define ALAC_DLL_EXPORT __attribute__((visibility("default")))
#define ALAC_DLL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(ALAC_DYNAMIC_IMPORT)
#define ALAC_EXPORT ALAC_DLL_IMPORT
#elif defined(ALAC_DYNAMIC)
#define ALAC_EXPORT ALAC_DLL_EXPORT
#else
#define ALAC_EXPORT
#endif
