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

#ifndef RESQUN_SVP_BEGIN
//! Name of the savepoint used in `begin` command.
#define RESQUN_SVP_BEGIN    RESQUN_PREFIX "begin_svp"
#endif // RESQUN_SVP_BEGIN

#ifndef RESQUN_SVP_UNDO
//! Name of the savepoint used in `undo` command.
#define RESQUN_SVP_UNDO    RESQUN_PREFIX "und_svp"
#endif // RESQUN_SVP_UNDO

#ifndef RESQUN_MARK_UNDO
//! Marker used in temporary table to indicate an UNDO entry.
#define RESQUN_MARK_UNDO    "'U'"
#endif // RESQUN_MARK_UNDO

#ifndef RESQUN_MARK_REDO
//! Marker used in temporary table to indicate an REDO entry.
#define RESQUN_MARK_REDO    "'R'"
#endif // RESQUN_MARK_REDO




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

public:

    //! How to deal with updates.
    enum UpdateBehaviour {
        NoTriggerForUpdate         = 0, /**< updates are not tracked; inserts
                                             and deletions are still tracked */
        OneTriggerPerUpdatedTable  = 1, /**< create a single trigger
                                             for this table */
        OneTriggerPerUpdatedColumn = 2  /**< create one trigger for each
                                             column of the table */
    };

    typedef int SqLiteResult;

    /*  DEFINITIONS    ===================================================== */
    //
    //
    //
    //
    /*  DATA    ------------------------------------------------------------ */

public:

    void * db_; /**< the actual sqlite database */
    bool is_active_;

    static const char * s_prefix_; /**< The prefix used by all methods
        inside sqlite environment. */
    static const char * s_fun_table_; /**< Name of the function used for adding
        a table to thelist monitored by this module. */

    static const char * s_sql_ur_count_; /**< sql statement for finding out
        how many entries are in the table. */
    static const char * s_sql_u_maxid_; /**< sql statement for finding out
        last id of the undo kind. */
    static const char * s_sql_r_maxid_; /**< sql statement for finding out
        last id of the redo kind. */

    static const char * s_sql_u_ins_; /**< sql statement for inserting
        a new entry of the undo kind. */
    static const char * s_sql_r_ins_; /**< sql statement for inserting
        a new entry of the redo kind. */


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

    //! Prepare the database to track this table.
    bool
    attachToTable (
            void *ctx,
            const QString & table,
            UpdateBehaviour update_kind);

    //! Get the number of entries in the temporary table by kind.
    SqLiteResult
    count (
            qint64 & undo_entries,
            qint64 & redo_entries) const;

    //! Get the rowid of the last recorded undo or redo step.
    SqLiteResult
    lastStepId (
            bool for_undo,
            qint64 & result) const;

    //! Read the sql statement from the temporary table.
    SqLiteResult
    getSqlStatementForId (
            qint64 rowid,
            QString &result) const;

    //! Read the last sql statement from the temporary table.
    SqLiteResult
    getLastSqlStatement (
            QString &result) const;

    //! Delete a record from the temporary table.
    SqLiteResult
    deleteForId (
            qint64 rowid) const;

    //! Insert a new record.
    SqLiteResult
    insertNew (
            bool for_undo) const;

    //! Creates a restore point.
    ReSqliteUn::SqLiteResult
    begin ();

    //! Closes a restore point.
    ReSqliteUn::SqLiteResult
    end ();

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

    //! The sql statements that create triggers for a table.
    QString
    sqlTriggers (
            const QString &table,
            UpdateBehaviour update_kind);

    //! Compute the sql string for insert trigger.
    QString
    sqlInsertTrigger (
            const QString &s_table);

    //! Compute the sql string for delete trigger.
    QString
    sqlDeleteTrigger (
            const QString &s_table,
            const QString & s_column_names,
            const QString & s_column_values);

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
