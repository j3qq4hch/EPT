#ifndef SNPRINTF_COMPAT_H_
#define SNPRINTF_COMPAT_H_

// Define __SNPRINTF before including this header to override (e.g. a
// smaller/custom snprintf implementation). Otherwise defaults to the
// standard library's snprintf.
#ifndef __SNPRINTF
#include <stdio.h>
#define __SNPRINTF snprintf
#endif

#endif // SNPRINTF_COMPAT_H_
