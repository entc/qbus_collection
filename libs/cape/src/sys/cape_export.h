#ifndef __CAPE_SYS__EXPORT__H
#define __CAPE_SYS__EXPORT__H 1

//----------------------------------------------------------------------------------
#ifdef __cplusplus
  #define __EXTERN_C    extern "C"
#else
  #define __EXTERN_C
#endif

//----------------------------------------------------------------------------------
// Windows
//----------------------------------------------------------------------------------

#if defined(_WIN64) || defined(_WIN32)

#define __WINDOWS_OS 1

#define __CAPE_LIBEX __EXTERN_C __declspec(dllexport)
#define __CAPE_CLASS __declspec(dllexport)
#define __CAPE_LOCAL __EXTERN_C

#define __STDCALL __stdcall

//----------------------------------------------------------------------------------
// ESP32 / ESP-IDF
//----------------------------------------------------------------------------------

#elif defined(ESP_PLATFORM)

#define CAPE_USE_FREERTOS 1

#define __CAPE_LIBEX __EXTERN_C
#define __CAPE_CLASS
#define __CAPE_LOCAL __EXTERN_C

#define __STDCALL

//----------------------------------------------------------------------------------
// Apple
//----------------------------------------------------------------------------------

#elif defined(__APPLE__)

#define __BSD_OS 1

#define __CAPE_LIBEX __EXTERN_C
#define __CAPE_CLASS
#define __CAPE_LOCAL __EXTERN_C __attribute__ ((visibility ("default")))

#define __STDCALL

//----------------------------------------------------------------------------------
// BSD
//----------------------------------------------------------------------------------

#elif defined(__bsdi__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#define __BSD_OS 1

#define __CAPE_LIBEX __EXTERN_C
#define __CAPE_CLASS
#define __CAPE_LOCAL __EXTERN_C

#define __STDCALL

//----------------------------------------------------------------------------------
// Linux
//----------------------------------------------------------------------------------

#elif defined(__linux__)

#define __LINUX_OS 1

#define __CAPE_LIBEX __EXTERN_C
#define __CAPE_CLASS
#define __CAPE_LOCAL __EXTERN_C

#define __STDCALL

#ifdef __GNUC__
#define _GNU_SOURCE 1
#endif

//----------------------------------------------------------------------------------
// Unknown
//----------------------------------------------------------------------------------

#else

#error "Unsupported operating system"

#endif

//----------------------------------------------------------------------------------
// Common
//----------------------------------------------------------------------------------

#define TRUE 1
#define FALSE 0

#endif
