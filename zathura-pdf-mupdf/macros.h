/* See LICENSE file for license and copyright information */

#ifndef MACROS_H
#define MACROS_H

#ifndef UNUSED
# if defined(__GNUC__) || defined(__clang__)
#  define UNUSED(x) UNUSED_ ## x __attribute__((unused))
# elif defined(__LCLINT__)
#  define UNUSED(x) /*@unused@*/ x
# else
#  define UNUSED(x) x
# endif
#endif

#ifndef DO_PRAGMA
# if defined(__GNUC__) || defined(__clang__)
#  define DO_PRAGMA(x) _Pragma(#x)
# else
#  define DO_PRAGMA(x)
# endif
#endif

#ifndef IGNORE_UNUSED_PARAMETER_BEGIN
#define IGNORE_UNUSED_PARAMETER_BEGIN \
  DO_PRAGMA(GCC diagnostic push) \
  DO_PRAGMA(GCC diagnostic ignored "-Wunused-parameter")
#endif

#ifndef IGNORE_UNUSED_PARAMETER_END
#define IGNORE_UNUSED_PARAMETER_END \
  DO_PRAGMA(GCC diagnostic pop)
#endif

#ifndef LENGTH
#define LENGTH(x) (sizeof(x)/sizeof((x)[0]))
#endif

#endif /* MACROS_H */
