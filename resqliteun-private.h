/**
 * @file resqliteun-private.h
 * @brief Declarations for ReSqliteUn class
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_RESQLITEUN_PRIVATE_H_INCLUDE
#define GUARD_RESQLITEUN_PRIVATE_H_INCLUDE

#include <resqliteun/resqliteun-config.h>

#if 0
#    define RESQLITEUN_DEBUGM printf
#else
#    define RESQLITEUN_DEBUGM black_hole
#endif

#if 0
#    define RESQLITEUN_TRACE_ENTRY printf("RESQLITEUN ENTRY %s in %s[%d]\n", __func__, __FILE__, __LINE__)
#else
#    define RESQLITEUN_TRACE_ENTRY
#endif

#if 0
#    define RESQLITEUN_TRACE_EXIT printf("RESQLITEUN EXIT %s in %s[%d]\n", __func__, __FILE__, __LINE__)
#else
#    define RESQLITEUN_TRACE_EXIT
#endif


static inline void black_hole (...)
{}

#endif // GUARD_RESQLITEUN_PRIVATE_H_INCLUDE
