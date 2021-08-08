#pragma once

#if defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

#define MSF_DECL_EXPORT Q_DECL_EXPORT
#define MSF_DECL_EXPORT Q_DECL_IMPORT

#else  // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/*
 * Compiler & system detection for IPC_DECL_EXPORT & MSF_DECL_EXPORT.
 * Not using QtCore cause it shouldn't depend on Qt.
 */

#if defined(_MSC_VER)
#define IPC_DECL_EXPORT __declspec(dllexport)
#define MSF_DECL_EXPORT __declspec(dllimport)
#elif defined(__ARMCC__) || defined(__CC_ARM)
#if defined(ANDROID) || defined(__linux__) || defined(__linux)
#define IPC_DECL_EXPORT __attribute__((visibility("default")))
#define MSF_DECL_EXPORT __attribute__((visibility("default")))
#else
#define IPC_DECL_EXPORT __declspec(dllexport)
#define MSF_DECL_EXPORT __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || \
    defined(__NT__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#define IPC_DECL_EXPORT __declspec(dllexport)
#define MSF_DECL_EXPORT __declspec(dllimport)
#else
#define IPC_DECL_EXPORT __attribute__((visibility("default")))
#define MSF_DECL_EXPORT __attribute__((visibility("default")))
#endif
#else
#define IPC_DECL_EXPORT __attribute__((visibility("default")))
#define MSF_DECL_EXPORT __attribute__((visibility("default")))
#endif

#endif  // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/*
 * Define MSF_EXPORT for exporting function & class.
 */

#ifndef MSF_EXPORT
#if defined(__MSF_LIBRARY__)
#define MSF_EXPORT IPC_DECL_EXPORT
#else
#define MSF_EXPORT MSF_DECL_EXPORT
#endif
#endif /*MSF_EXPORT*/
