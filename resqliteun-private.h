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
#include <QDebug>

#if 1
//#    define RESQLITEUN_DEBUGM printf
#    define RESQLITEUN_DEBUGM qDebug() << QString::asprintf
#else
#    define RESQLITEUN_DEBUGM black_hole
#endif

#if 1
//#    define RESQLITEUN_TRACE_ENTRY printf("RESQLITEUN ENTRY %s in %s[%d]\n", __func__, __FILE__, __LINE__)
#    define RESQLITEUN_TRACE_ENTRY qDebug() << QString::asprintf("RESQLITEUN ENTRY %s", __func__)
#else
#    define RESQLITEUN_TRACE_ENTRY
#endif

#if 1
//#    define RESQLITEUN_TRACE_EXIT printf("RESQLITEUN EXIT %s in %s[%d]\n", __func__, __FILE__, __LINE__)
#    define RESQLITEUN_TRACE_EXIT qDebug() << QString::asprintf("RESQLITEUN EXIT %s", __func__)
#else
#    define RESQLITEUN_TRACE_EXIT
#endif


static inline void black_hole (...)
{}

#endif // GUARD_RESQLITEUN_PRIVATE_H_INCLUDE
