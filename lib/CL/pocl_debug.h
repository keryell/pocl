#ifndef POCL_DEBUG_H
#define POCL_DEBUG_H

#ifdef _WIN32
#  include <stdint.h>
#  include <stddef.h> // size_t
#  define PRIu64 "I64u"
#  define PRIX64 "I64x"
#  define PRIXPTR "p"
#  define PRIuS "Iu"
#else
# ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS
# endif
# include <inttypes.h>
#endif

// size_t print spec
#ifndef PRIuS
# define PRIuS "zu"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// should use some terminfo library, but..
#define POCL_COLOR_RESET   "\033[0m"
#define POCL_COLOR_BLACK   "\033[30m"      /* Black */
#define POCL_COLOR_RED     "\033[31m"      /* Red */
#define POCL_COLOR_GREEN   "\033[32m"      /* Green */
#define POCL_COLOR_YELLOW  "\033[33m"      /* Yellow */
#define POCL_COLOR_BLUE    "\033[34m"      /* Blue */
#define POCL_COLOR_MAGENTA "\033[35m"      /* Magenta */
#define POCL_COLOR_CYAN    "\033[36m"      /* Cyan */
#define POCL_COLOR_WHITE   "\033[37m"      /* White */
#define POCL_COLOR_BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define POCL_COLOR_BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define POCL_COLOR_BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define POCL_COLOR_BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define POCL_COLOR_BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define POCL_COLOR_BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define POCL_COLOR_BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define POCL_COLOR_BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

/* bitfield values for pocl_debug_messages_filter */
#define POCL_DEBUG_FLAG_GENERAL 0x1
#define POCL_DEBUG_FLAG_MEMORY 0x2
#define POCL_DEBUG_FLAG_LLVM 0x4
#define POCL_DEBUG_FLAG_EVENTS 0x8
#define POCL_DEBUG_FLAG_CACHE 0x10
#define POCL_DEBUG_FLAG_LOCKING 0x20
#define POCL_DEBUG_FLAG_REFCOUNTS 0x40
#define POCL_DEBUG_FLAG_TIMING 0x80
#define POCL_DEBUG_FLAG_HSA 0x100
#define POCL_DEBUG_FLAG_TCE 0x200
#define POCL_DEBUG_FLAG_CUDA 0x400
#define POCL_DEBUG_FLAG_ALL (uint64_t)(-1)

#define POCL_FILTER_TYPE_INFO 1
#define POCL_FILTER_TYPE_WARN 2
#define POCL_FILTER_TYPE_ERR 3


#ifdef __GNUC__
#pragma GCC visibility push(hidden)
#endif

/* Debugging macros. Also macros for marking unimplemented parts of specs or
   untested parts of the implementation. */

extern int pocl_aborting;

#define POCL_ABORT_UNIMPLEMENTED(MSG)                                   \
    do {                                                                \
        fprintf(stderr,"%s is unimplemented (%s:%d)\n",                 \
                        MSG, __FILE__, __LINE__);                       \
        exit(2);                                                        \
    } while (0)

#define POCL_WARN_UNTESTED()                                            \
    do {                                                                \
        fprintf( stderr,                                                \
            "pocl warning: encountered untested part of the "           \
            "implementation in %s:%d\n", __FILE__, __LINE__);           \
    } while (0)

#define POCL_WARN_INCOMPLETE()                                          \
    do {                                                                \
        fprintf( stderr,                                                \
            "pocl warning: encountered incomplete implementation"       \
            " in %s:%d\n", __FILE__, __LINE__);                         \
    } while (0)

#define POCL_ABORT(...)                                                 \
    do {                                                                \
        pocl_aborting = 1;                                              \
        fprintf(stderr, __VA_ARGS__);                                   \
        abort();                                                        \
    } while (0)

#define POCL_ERROR(x) do { if (errcode_ret != NULL) {                   \
                              *errcode_ret = (x);                       \
                           } return NULL; } while (0)

#define POCL_SUCCESS() do { if (errcode_ret != NULL) {                  \
                              *errcode_ret = CL_SUCCESS;                \
                            } } while (0)



#include "config.h"

#ifdef POCL_DEBUG_MESSAGES

    extern uint64_t pocl_debug_messages_filter;
    extern int stderr_is_a_tty;

    #define POCL_DEBUGGING_ON (pocl_debug_messages_filter)

    #if __GNUC__ >= 2
    #define __func__ __PRETTY_FUNCTION__
    #else
    #define __func__ __FUNCTION__
    #endif

        int pocl_fprintf_err (const char* format, ...);
        #define POCL_DEBUG_HEADER(FILTER, FILTER_TYPE) \
            pocl_debug_print_header (__func__, __LINE__, #FILTER, FILTER_TYPE);
        extern void pocl_debug_messages_setup (const char *debug);
        extern void pocl_debug_print_header (const char * func, unsigned line,
                                             const char* filter, int filter_type);
        extern void pocl_debug_measure_start (uint64_t* start);
        extern void pocl_debug_measure_finish (uint64_t* start, uint64_t* finish,
                                               const char* msg,
                                               const char *func,
                                               unsigned line);
        extern void pocl_debug_print_duration (const char* func, unsigned line,
                                               const char* msg, uint64_t nanosecs);
        #define POCL_MEASURE_START(SUFFIX) \
          uint64_t pocl_time_start_ ## SUFFIX, pocl_time_finish_ ## SUFFIX; \
          pocl_debug_measure_start(&pocl_time_start_ ## SUFFIX);

        #define POCL_MEASURE_FINISH(SUFFIX) \
          pocl_debug_measure_finish(&pocl_time_start_ ## SUFFIX, \
                         &pocl_time_finish_ ## SUFFIX, "API: " #SUFFIX, \
                         __func__, __LINE__);

    #define POCL_MSG_PRINT_F(FILTER, TYPE, ERRCODE, ...)                    \
        do {                                                                \
            if (pocl_debug_messages_filter & POCL_DEBUG_FLAG_ ## FILTER) {  \
                POCL_DEBUG_HEADER(FILTER, POCL_FILTER_TYPE_ ## TYPE)        \
                if (stderr_is_a_tty)                                        \
                  pocl_fprintf_err ("%s", POCL_COLOR_BOLDRED                \
                                    ERRCODE " "  POCL_COLOR_RESET);         \
                else                                                        \
                  pocl_fprintf_err ("%s", ERRCODE " ");                     \
                pocl_fprintf_err (__VA_ARGS__);                             \
            }                                                               \
        } while (0)

    #define POCL_MSG_PRINT2(FILTER, func, line, ...)                        \
        do {                                                                \
            if (pocl_debug_messages_filter & POCL_DEBUG_FLAG_ ## FILTER) {  \
                pocl_debug_print_header (func, line,                        \
                                 #FILTER, POCL_FILTER_TYPE_INFO);           \
                pocl_fprintf_err (__VA_ARGS__);                             \
            }                                                               \
        } while (0)

    #define POCL_MSG_PRINT(TYPE, ERRCODE, ...)                              \
            POCL_MSG_PRINT_F(GENERAL, TYPE, ERRCODE, __VA_ARGS__)

    #define POCL_MSG_WARN2(errcode, ...) \
              POCL_MSG_PRINT(WARN, errcode, __VA_ARGS__)
    #define POCL_MSG_WARN(...)  POCL_MSG_WARN2("", __VA_ARGS__)

    #define POCL_MSG_ERR2(errcode, ...) \
          POCL_MSG_PRINT(ERR, errcode, __VA_ARGS__)
    #define POCL_MSG_ERR(...)  POCL_MSG_ERR2("", __VA_ARGS__)

    #define POCL_MSG_PRINT_INFO2(errcode, ...) \
          POCL_MSG_PRINT(INFO, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_INFO(...) POCL_MSG_PRINT_INFO2("", __VA_ARGS__)

    #define POCL_MSG_PRINT_INFO_F(filter, errcode, ...) \
          POCL_MSG_PRINT_F(filter, INFO, errcode, __VA_ARGS__)

    #define POCL_MSG_PRINT_CUDA2(errcode, ...) POCL_MSG_PRINT_INFO_F(CUDA, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_CUDA(...) POCL_MSG_PRINT_INFO_F(CUDA, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_HSA2(errcode, ...) POCL_MSG_PRINT_INFO_F(HSA, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_HSA(...) POCL_MSG_PRINT_INFO_F(HSA, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_TCE2(errcode, ...) POCL_MSG_PRINT_INFO_F(TCE, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_TCE(...) POCL_MSG_PRINT_INFO_F(TCE, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_LOCKING2(errcode, ...) POCL_MSG_PRINT_INFO_F(LOCKING, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_LOCKING(...) POCL_MSG_PRINT_INFO_F(LOCKING, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_REFCOUNTS2(errcode, ...) POCL_MSG_PRINT_INFO_F(REFCOUNTS, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_REFCOUNTS(...) POCL_MSG_PRINT_INFO_F(REFCOUNTS, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_CACHE2(errcode, ...) POCL_MSG_PRINT_INFO_F(CACHE, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_CACHE(...) POCL_MSG_PRINT_INFO_F(CACHE, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_EVENTS2(errcode, ...) POCL_MSG_PRINT_INFO_F(EVENTS, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_EVENTS(...) POCL_MSG_PRINT_INFO_F(EVENTS, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_LLVM2(errcode, ...) POCL_MSG_PRINT_INFO_F(LLVM, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_LLVM(...) POCL_MSG_PRINT_INFO_F(LLVM, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_MEMORY2(errcode, ...) POCL_MSG_PRINT_INFO_F(MEMORY, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_MEMORY(...) POCL_MSG_PRINT_INFO_F(MEMORY, "", __VA_ARGS__)
    #define POCL_MSG_PRINT_GENERAL2(errcode, ...) POCL_MSG_PRINT_INFO_F(GENERAL, errcode, __VA_ARGS__)
    #define POCL_MSG_PRINT_GENERAL(...) POCL_MSG_PRINT_INFO_F(GENERAL, "", __VA_ARGS__)

    #define POCL_DEBUG_EVENT_TIME(eventp, msg) \
        pocl_debug_print_duration(__func__, __LINE__, "Event " msg, (uint64_t)((*eventp)->time_end - (*eventp)->time_start))

#else

    #define POCL_DEBUGGING_ON 0

    #define POCL_MSG_PRINT_F(...)
    #define POCL_MSG_PRINT(...)
    #define POCL_MSG_PRINT2(...)
    #define POCL_MSG_WARN(...)
    #define POCL_MSG_WARN2(...)
    #define POCL_MSG_ERR(...)
    #define POCL_MSG_ERR2(...)
    #define POCL_MSG_PRINT_INFO(...)
    #define POCL_MSG_PRINT_INFO2(...)
    #define POCL_MSG_PRINT_INFO_F(...)

    #define POCL_DEBUG_HEADER
    #define POCL_MEASURE_START(...)
    #define POCL_MEASURE_FINISH(...)
    #define POCL_DEBUG_EVENT_TIME(...)

    #define POCL_MSG_PRINT_HSA2(...)
    #define POCL_MSG_PRINT_HSA(...)
    #define POCL_MSG_PRINT_TCE2(...)
    #define POCL_MSG_PRINT_TCE(...)
    #define POCL_MSG_PRINT_LOCKING2(...)
    #define POCL_MSG_PRINT_LOCKING(...)
    #define POCL_MSG_PRINT_REFCOUNTS2(...)
    #define POCL_MSG_PRINT_REFCOUNTS(...)
    #define POCL_MSG_PRINT_CACHE2(...)
    #define POCL_MSG_PRINT_CACHE(...)
    #define POCL_MSG_PRINT_EVENTS2(...)
    #define POCL_MSG_PRINT_EVENTS(...)
    #define POCL_MSG_PRINT_LLVM2(...)
    #define POCL_MSG_PRINT_LLVM(...)
    #define POCL_MSG_PRINT_MEMORY2(...)
    #define POCL_MSG_PRINT_MEMORY(...)
    #define POCL_MSG_PRINT_GENERAL2(...)
    #define POCL_MSG_PRINT_GENERAL(...)

#endif


#define POCL_GOTO_ERROR_ON(cond, err_code, ...)                             \
  do                                                                        \
    {                                                                       \
      if (cond)                                                             \
        {                                                                   \
            POCL_MSG_ERR2(#err_code, __VA_ARGS__);                          \
            errcode = err_code;                                             \
            goto ERROR;                                                     \
        }                                                                   \
    }                                                                       \
  while (0)

#define POCL_RETURN_ERROR_ON(cond, err_code, ...)                           \
  do                                                                        \
    {                                                                       \
      if (cond)                                                             \
        {                                                                   \
            POCL_MSG_ERR2(#err_code, __VA_ARGS__);                          \
            return err_code;                                                \
        }                                                                   \
    }                                                                       \
  while (0)

#define POCL_RETURN_ERROR_COND(cond, err_code)                              \
  do                                                                        \
    {                                                                       \
      if (cond)                                                             \
        {                                                                   \
          POCL_MSG_ERR2(#err_code, "%s\n", #cond);                          \
          return err_code;                                                  \
        }                                                                   \
    }                                                                       \
  while (0)

#define POCL_GOTO_ERROR_COND(cond, err_code)                                \
  do                                                                        \
    {                                                                       \
      if (cond)                                                             \
        {                                                                   \
          POCL_MSG_ERR2(#err_code, "%s\n", #cond);                          \
          errcode = err_code;                                               \
          goto ERROR;                                                       \
        }                                                                   \
    }                                                                       \
  while (0)



#ifdef __GNUC__
#pragma GCC visibility pop
#endif

#ifdef __cplusplus
}
#endif

#endif
