/* ========================================================================= */
/* ------------------------------------------------------------------------- */
/**
 * @file resqliteun2.cc
 * @brief Definitions for ReSqliteUn class.
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */
/* ------------------------------------------------------------------------- */
/* ========================================================================= */
//
//
//
//
/*  INCLUDES    ------------------------------------------------------------ */

#include <sqlite/sqlite3.h>

#include "resqliteun.h"
#include "resqliteun-private.h"

#include <assert.h>

/*  INCLUDES    ============================================================ */
//
//
//
//
/*  DEFINITIONS    --------------------------------------------------------- */

const char * ReSqliteUn::s_prefix_ = RESQUN_PREFIX;
const char * ReSqliteUn::s_fun_table_ = RESQUN_FUN_TABLE;

ReSqliteUn * ReSqliteUn::instance_ = NULL;

typedef void(*xEntryPoint)(void);
xEntryPoint getEntryPoint ();

/*  DEFINITIONS    ========================================================= */
//
//
//
//
/*  CLASS    --------------------------------------------------------------- */

/**
 * @class ReSqliteUn
 *
 * The class contains the implementation for the undo-redo plugin for
 * sqlite. One of these instances will be attached to each database.
 *
 * The plugin will attach an instance of the ReSqliteUn class to each
 * function but only the `table` function will delete this pointer. Given that
 * sqlite calls the `destroy` function if the `table` function is overloaded
 * (and when the database connection is closed) the end result is that our
 * `table` function can NOT be overloaded.
 */

/* ------------------------------------------------------------------------- */
/**
 * Detailed description for constructor.
 */
ReSqliteUn::ReSqliteUn (
        void *db) :
    db_ (db)
{
    RESQLITEUN_TRACE_ENTRY;

    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * Detailed description for destructor.
 */
ReSqliteUn::~ReSqliteUn()
{
    RESQLITEUN_TRACE_ENTRY;

    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * Detailed description for destructor.
 */
bool ReSqliteUn::attachToTable ()
{
    RESQLITEUN_TRACE_ENTRY;
    bool b_ret = false;
    for (;;) {



        b_ret = true;
        break;
    }
    RESQLITEUN_TRACE_EXIT;
    return b_ret;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
bool ReSqliteUn::autoregister ()
{
    int rc = sqlite3_auto_extension (getEntryPoint ());
    return (rc == SQLITE_OK);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
QString ReSqliteUn::columnText (void *statement, int column)
{
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(statement);
    return QString(reinterpret_cast<const QChar *>(
                       sqlite3_column_text16 (stmt, column)),
                       sqlite3_column_bytes16 (stmt, column) / sizeof(QChar));
}
/* ========================================================================= */

/*  CLASS    =============================================================== */
//
//
//
//
/* ------------------------------------------------------------------------- */
/* ========================================================================= */
