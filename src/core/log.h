#ifndef PUMICE_LOG_H
#define PUMICE_LOG_H

#include <stdio.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif

#if LOG_LEVEL >= 1
#define LOG_ERR(...)  (fprintf(stderr, "[pumice] ERROR: " __VA_ARGS__), fputc('\n', stderr))
#else
#define LOG_ERR(...)  ((void)0)
#endif

#if LOG_LEVEL >= 2
#define LOG_INFO(...) (fprintf(stderr, "[pumice] " __VA_ARGS__), fputc('\n', stderr))
#else
#define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL >= 3
#define LOG_TRACE(...) (fprintf(stderr, "[pumice] TRACE: " __VA_ARGS__), fputc('\n', stderr))
#else
#define LOG_TRACE(...) ((void)0)
#endif

#endif
