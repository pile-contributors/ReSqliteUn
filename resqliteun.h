/* ========================================================================= */
/* ------------------------------------------------------------------------- */
/**
 * @file resqliteun.h
 * @brief Declarations for ReSqliteUn class
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_RESQLITEUN_H_INCLUDE
#define GUARD_RESQLITEUN_H_INCLUDE

/* ------------------------------------------------------------------------- */
/* ========================================================================= */
//
//
//
//
/*  INCLUDES    ------------------------------------------------------------ */

#include <resqliteun/resqliteun-config.h>
#include <QString>

/*  INCLUDES    ============================================================ */
//
//
//
//
/*  DEFINITIONS    --------------------------------------------------------- */

/**
 * @defgroup RESQUN_STRINGS text strings used by the module
 *
 * For convenience the strings used by the plugin that might be useful
 * to the user are defined here; they are also available as static
 * members in the ReSqliteUn class.
 * @{
 */

#ifndef RESQUN_PREFIX
//! The prefix used by all methods inside sqlite environment.
#define RESQUN_PREFIX       "resqun_"
#endif // RESQUN_PREFIX

#ifndef RESQUN_FUN_TABLE
//! Name of the function used for adding a table to the list monitored by this module.
#define RESQUN_FUN_TABLE    RESQUN_PREFIX "table"
#endif // RESQUN_FUN_TABLE

#ifndef RESQUN_FUN_ACTIVE
//! Name of the function used for
#define RESQUN_FUN_ACTIVE   RESQUN_PREFIX "active"
#endif // RESQUN_FUN_ACTIVE

#ifndef RESQUN_FUN_BEGIN
//! Name of the function used for
#define RESQUN_FUN_BEGIN    RESQUN_PREFIX "begin"
#endif // RESQUN_FUN_BEGIN

#ifndef RESQUN_FUN_END
//! Name of the function used for .
#define RESQUN_FUN_END      RESQUN_PREFIX "end"
#endif // RESQUN_FUN_END

#ifndef RESQUN_FUN_UNDO
//! Name of the function used for performing an undo step.
#define RESQUN_FUN_UNDO     RESQUN_PREFIX "undo"
#endif // RESQUN_FUN_UNDO

#ifndef RESQUN_FUN_REDO
//! Name of the function used for performing an redo step.
#define RESQUN_FUN_REDO     RESQUN_PREFIX "redo"
#endif // RESQUN_FUN_REDO

#ifndef RESQUN_TBL_TEMP
//! The table to be used for storing undo-redo stack.
#define RESQUN_TBL_TEMP     RESQUN_PREFIX "sqlite_undo"
#endif // RESQUN_TBL_TEMP

/** @} */


/*  DEFINITIONS    ========================================================= */
//
//
//
//
/*  CLASS    --------------------------------------------------------------- */

//! Adds undo-redo capabilities to a sqlite database.
class RESQLITEUN_EXPORT ReSqliteUn {
    //
    //
    //
    //
    /*  DEFINITIONS    ----------------------------------------------------- */

    /*  DEFINITIONS    ===================================================== */
    //
    //
    //
    //
    /*  DATA    ------------------------------------------------------------ */

public:

    void * db_; /**< the actual sqlite database */

    static const char * s_prefix_; /**< The prefix used by all methods
        inside sqlite environment. */
    static const char * s_fun_table_; /**< Name of the function used for adding
        a table to thelist monitored by this module. */

    static ReSqliteUn * instance_; /**< the one and only instance */

    /*  DATA    ============================================================ */
    //
    //
    //
    //
    /*  FUNCTIONS    ------------------------------------------------------- */

public:

    //! Default constructor.
    ReSqliteUn (
            void *db);

    //! Destructor.
    virtual ~ReSqliteUn();


    bool
    attachToTable();

    //! Prepare the database to track this table.
    bool
    attachToTable (
            const QString & table,
            int flags);

public:

    //! The one and only instance.
    static ReSqliteUn *
    instance () {
        return instance_;
    }

    //! Autoregister this extension with each new database (when not using the plugin).
    static bool
    autoregister();

    static QString
    columnText (
            void * statement,
            int column);

protected:

private:

    //! Compute the sql string for insert trigger.
    QString
    sqlInsertTrigger (
            const QString &s_table);

    //! Compute the sql string for delete trigger.
    QString
    sqlDeleteTrigger (
            const QString &s_table,
            const QString &s_column_list);

    //! Compute the sql string for update trigger.
    QString
    sqlUpdateTriggerPerColumn (
            const QString &s_table,
            const QString &s_colum);

    //! Compute the sql string for update trigger.
    QString
    sqlUpdateTriggerPerTable (
            const QString &s_table,
            const QString &s_column_list);

    /*  FUNCTIONS    ======================================================= */
    //
    //
    //
    //

}; // class ReSqliteUn

/*  CLASS    =============================================================== */
//
//
//
//

#endif // GUARD_RESQLITEUN_H_INCLUDE

/* ------------------------------------------------------------------------- */
/* ========================================================================= */
