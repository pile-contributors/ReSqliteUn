/* ========================================================================= */
/* ------------------------------------------------------------------------- */
/**
 * @file resqliteun-manager.cc
 * @brief Definitions for ReSqliteUnManager class.
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
#include <QStringBuilder>

/*  INCLUDES    ============================================================ */
//
//
//
//
/*  DEFINITIONS    --------------------------------------------------------- */

// the list of instances
QList<ReSqliteUn *> ReSqliteUnManager::instances_;

/*  DEFINITIONS    ========================================================= */
//
//
//
//
/*  CLASS    --------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/**
 *
 * @param interface_version Internal use (checks that the version is teh right one).
 * @return NULL if there is no database.
 */
ReSqliteUn *ReSqliteUnManager::instance (int interface_version)
{
    if (interface_version != RESQLITEUN_VERSION) {
        fprintf(stderr, "Version missmatch; library version is %#010x, "
                        "interface version is %#010x",
                RESQLITEUN_VERSION, interface_version);
        return NULL;
    }

    int last = instances_.count () - 1;
    if (last < 0) {
        return NULL;
    } else {
        return instances_.at (last);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * The index of a database changes when a database created befoore this one
 * is closed (the destructor removes instances from the list).
 *
 * @param i The index in internal list where valid range is 0 .. count()-1
 * @param interface_version Internal use (checks that the version is teh right one).
 * @return NULL if index is invalid.
 */
ReSqliteUn *ReSqliteUnManager::instanceForIndex (
        int i, int interface_version)
{
    if (interface_version != RESQLITEUN_VERSION) {
        fprintf(stderr, "Version missmatch; library version is %#010x, "
                        "interface version is %#010x",
                RESQLITEUN_VERSION, interface_version);
        return NULL;
    }

    if ((i < 0) || (i >= instances_.count ())) {
        return NULL;
    } else {
        return instances_.at (i);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * The method involves checking all instances so it may be worth caching
 * the result if this is called often.
 *
 * @param sqlite_database The database to look for.
 * @param interface_version Internal use (checks that the version is teh right one).
 * @return NULL if no such database is inside.
 */
ReSqliteUn *ReSqliteUnManager::instanceForDatabase (
        void *sqlite_database, int interface_version)
{
    if (interface_version != RESQLITEUN_VERSION) {
        fprintf(stderr, "Version missmatch; library version is %#010x, "
                        "interface version is %#010x",
                RESQLITEUN_VERSION, interface_version);
        return NULL;
    }
    sqlite3 * dtb = static_cast<sqlite3 *>(sqlite_database);

    foreach(ReSqliteUn * iter, instances_) {
        if (dtb == static_cast<sqlite3 *>(iter->db_)) {
            return iter;
        }
    }

    return NULL;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
bool ReSqliteUnManager::autoregister ()
{
    RESQLITEUN_TRACE_ENTRY;
    int rc = sqlite3_auto_extension (getEntryPoint ());
    RESQLITEUN_TRACE_EXIT;
    return (rc == SQLITE_OK);
}
/* ========================================================================= */


/*  CLASS    =============================================================== */
//
//
//
//
/* ------------------------------------------------------------------------- */
/* ========================================================================= */

