/* ========================================================================= */
/* ------------------------------------------------------------------------- */
/**
 * @file resqliteun.cc
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
#include <QStringBuilder>

/*  INCLUDES    ============================================================ */
//
//
//
//
/*  DEFINITIONS    --------------------------------------------------------- */

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define dtb_ static_cast<sqlite3 *>(db_)

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
 *
 */

/* ------------------------------------------------------------------------- */
/**
 * Marks itself as the default instance (this is the instance that is returned,
 * odly enough, by the instance() method).
 */
ReSqliteUn::ReSqliteUn (
        void *db) :
    db_ (db),
    is_active_ (false),
    in_undo_(true)
{
    RESQLITEUN_TRACE_ENTRY;
    instances_.append (this);
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * If this is the default (last created) instnce then it will be no
 * default instance from this point forward..
 */
ReSqliteUn::~ReSqliteUn()
{
    RESQLITEUN_TRACE_ENTRY;
    instances_.removeOne (this);
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * This entry creates a new entry in the index table and removes all "redo"
 * entries that might exist with all their associated data.
 *
 * The instance is put on active state and will record future events.
 *
 * @param s_name The name to be inserted in the index table.
 * @return error code
 */
ReSqliteUn::SqLiteResult ReSqliteUn::begin (
        const QString & s_name, qint64 * entry_id)
{
    ReSqliteUn::SqLiteResult rc = SQLITE_ERROR;
    for (;;) {

        if (is_active_) {
            rc = SQLITE_MISUSE;
            break;
        }

        // Remove all "redo" entries and place a marker for first undo entry.
        QString statements = QString (
            "SAVEPOINT " RESQUN_SVP_BEGIN ";"

            // we're removing all data entries that belong to redo entries
            "DELETE FROM " RESQUN_TBL_TEMP " WHERE idxid IN ("
                "SELECT id FROM " RESQUN_TBL_IDX
                    " WHERE status=" STR(RESQUN_MARK_REDO) ");"

            // Then  we remove all redo entries.
            "DELETE FROM " RESQUN_TBL_IDX " WHERE status=" STR(RESQUN_MARK_REDO) ";"

            // And we're inserting a new undo entry.
            "INSERT INTO " RESQUN_TBL_IDX "(name, status) VALUES('%1'," STR(RESQUN_MARK_UNDO) ");"
            ).arg (s_name);
        rc = sqlite3_exec (
            dtb_,
            statements.toUtf8().constData (),
            NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_exec (dtb_,
                "ROLLBACK TO SAVEPOINT " RESQUN_SVP_BEGIN,
                NULL, NULL, NULL);
            rc = SQLITE_ERROR;
        } else {
            is_active_ = true;
            rc = SQLITE_OK;

            if (entry_id != NULL) {
                *entry_id = sqlite3_last_insert_rowid (dtb_);
            }
        }
        sqlite3_exec (dtb_, "RELEASE SAVEPOINT " RESQUN_SVP_BEGIN,
                NULL, NULL, NULL);

        in_undo_ = true;
        break;
    }
    return rc;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ReSqliteUn::SqLiteResult ReSqliteUn::end ()
{
    ReSqliteUn::SqLiteResult rc = SQLITE_ERROR;
    for (;;) {

        if (!is_active_) {
            rc = SQLITE_MISUSE;
            break;
        }

        is_active_ = false;

        rc = SQLITE_OK;
        break;
    }
    return rc;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * We're adding a table to the list of tables managed by the
 * undo-redo mechanism.
 */
ReSqliteUn::SqLiteResult ReSqliteUn::attachToTable (
        const QString & table, UpdateBehaviour update_kind)
{
    RESQLITEUN_TRACE_ENTRY;
    QString statements = sqlTriggers (db_, table, update_kind);
    // printf(statements.toLatin1().constData());

    // This is inefficient as toUtf8 will allocate a new buffer;
    // as sqlite3_exec only works with Utf8 (there is no 16 alternative)
    // we are left with three options: use utf8 all over the place,
    // the one below or reimplementing sqlite3_exec.
    // As the this function will probably used only when the program starts,
    // once for each table, the performance penality is neglijable IMHO.
    char * err_msg;
    ReSqliteUn::SqLiteResult rc = sqlite3_exec (
                dtb_,
                statements.toUtf8().constData (),
                NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to install triggers:\n%s\n\n%s\n",
                err_msg, statements.toLatin1().constData());
    }
    RESQLITEUN_TRACE_EXIT;
    return rc;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ReSqliteUnUtil::SqLiteResult deleteById (
        sqlite3 * database, quint64 the_id)
{
    ReSqliteUnUtil::SqLiteResult rc = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    for (;;) {

        rc = sqlite3_prepare_v2 (
                    database,
                    "DELETE "
                    " FROM " RESQUN_TBL_TEMP
                    " WHERE idxid=?",
                -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("deleteById(): prepare failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }
        rc = sqlite3_bind_int64 (stmt, 1, the_id);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("deleteById(): bind failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }
        rc = sqlite3_step (stmt);
        if (rc != SQLITE_DONE) {
            RESQLITEUN_DEBUGM("deleteById(): step failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }

        rc = SQLITE_OK;
        break;
    }
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    return rc;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ReSqliteUnUtil::SqLiteResult statementsById (
        sqlite3 * database, quint64 the_id, QString & result)
{
    ReSqliteUnUtil::SqLiteResult rc = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    for (;;) {

        rc = sqlite3_prepare_v2 (
                    database,
                    "SELECT group_concat(sql, ';')"
                    " FROM " RESQUN_TBL_TEMP
                    " WHERE idxid=?",
                -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("statementsById(): prepare failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }
        rc = sqlite3_bind_int64 (stmt, 1, the_id);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("statementsById(): bind failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }
        rc = sqlite3_step (stmt);
        if (rc != SQLITE_ROW) {
            RESQLITEUN_DEBUGM("statementsById(): step failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }

        result = ReSqliteUn::columnText (stmt, 0);

        rc = SQLITE_OK;
        break;
    }
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    return rc;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ReSqliteUnUtil::SqLiteResult changeStatusById (
        sqlite3 * database, quint64 the_id, int new_status)
{
    ReSqliteUnUtil::SqLiteResult rc = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    for (;;) {

        rc = sqlite3_prepare_v2 (
                    database,
                    "UPDATE " RESQUN_TBL_IDX
                    " SET status=?"
                    " WHERE id=?",
                -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("changeStatusById(): prepare failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }
        rc = sqlite3_bind_int (stmt, 1, new_status);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("changeStatusById(): bind failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }
        rc = sqlite3_bind_int64 (stmt, 2, the_id);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("changeStatusById(): bind failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }
        rc = sqlite3_step (stmt);
        if (rc != SQLITE_DONE) {
            RESQLITEUN_DEBUGM("changeStatusById(): step failed: %s\n",
                              sqlite3_errmsg(database));
            break;
        }

        rc = SQLITE_OK;
        break;
    }
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    return rc;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ReSqliteUnUtil::SqLiteResult ReSqliteUn::performUndoRedo (
        bool for_undo, QString &s_error)
{
    ReSqliteUnUtil::SqLiteResult rc = SQLITE_ERROR;
    bool rollback = false;
    for (;;) {
        if (is_active_) {
            rc = SQLITE_MISUSE;
            break;
        }

        // Get the id of the undo or redo entry.
        qint64 active = getActiveId (for_undo ? UndoType : RedoType);
        if (active < 1) {
            RESQLITEUN_DEBUGM("performUndoRedo(): getActiveId failed: %s\n",
                              sqlite3_errmsg(dtb_));
            rc = SQLITE_DONE;
            break;
        }

        // Get the statements needed to get the database to former glory.
        QString statements;
        rc = statementsById (dtb_, active, statements);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("performUndoRedo(): statementsById failed: %s\n",
                              sqlite3_errmsg(dtb_));
            break;
        }

        rc = sqlite3_exec (dtb_, "SAVEPOINT " RESQUN_SVP_UNDO,
                NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            break;
        }
        rollback = true; {

            // Delete all those old statements.
            rc = deleteById (dtb_, active);
            if (rc != SQLITE_OK) {
                RESQLITEUN_DEBUGM("performUndoRedo(): statementsById failed: %s\n",
                                  sqlite3_errmsg(dtb_));
                break;
            }

            // Switch the status from undo to redo and vv.
            rc = changeStatusById (
                        dtb_, active, for_undo ?
                            RESQUN_MARK_REDO : RESQUN_MARK_UNDO );
            if (rc != SQLITE_OK) {
                RESQLITEUN_DEBUGM("performUndoRedo(): changeStatusById failed: %s\n",
                                  sqlite3_errmsg(dtb_));
                break;
            }

            char * error_msg;
            in_undo_ = !for_undo;
            is_active_ = true; {
                rc = sqlite3_exec (
                            dtb_, statements.toUtf8 ().constData (),
                            NULL, NULL, &error_msg);
            } is_active_ = false;
            if (rc != SQLITE_OK) {
                s_error = tr("Cannot perform the update.\n%1").arg  (error_msg);
                break;
            }

        } rollback = false;
        sqlite3_exec (dtb_, "RELEASE SAVEPOINT " RESQUN_SVP_UNDO,
            NULL, NULL, NULL);

        break;
    }
    if (rollback) {
        sqlite3_exec (dtb_,
            "ROLLBACK TO SAVEPOINT " RESQUN_SVP_UNDO ";"
            "RELEASE SAVEPOINT " RESQUN_SVP_UNDO,
            NULL, NULL, NULL);
    }
    return rc;
}
/* ========================================================================= */


/* ------------------------------------------------------------------------- */
/**
 * @warning The result is the error code (SQLITE_OK if all went well).
 *
 * @param undo_entries Resulted undo entries
 * @param redo_entries Resulted redo entries
 * @return error code
 */
ReSqliteUn::SqLiteResult ReSqliteUn::count (
        qint64 &undo_entries, qint64 &redo_entries) const
{
    ReSqliteUn::SqLiteResult rc = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    for (;;) {
        rc = sqlite3_prepare_v2 (
                    dtb_,
                    "SELECT "
                    "(SELECT COUNT(*) FROM " RESQUN_TBL_IDX " "
                        "WHERE status=" STR(RESQUN_MARK_UNDO) ") AS UNDO,"
                    "(SELECT COUNT(*) FROM " RESQUN_TBL_IDX " "
                        "WHERE status=" STR(RESQUN_MARK_REDO) ") AS REDO;",
                -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("cunt(): prepare failed: %s\n",
                              sqlite3_errmsg(dtb_));
            break;
        }

        rc = sqlite3_step (stmt);
        if (rc != SQLITE_ROW) {
            RESQLITEUN_DEBUGM("cunt(): step failed: %s\n",
                              sqlite3_errmsg(dtb_));
            break;
        }

        undo_entries = sqlite3_column_int64 (stmt, 0);
        redo_entries = sqlite3_column_int64 (stmt, 1);

        rc = SQLITE_OK;
        break;
    }

    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    return rc;
}
/* ========================================================================= */


/* ------------------------------------------------------------------------- */
/**
 * This method retrieves the current record based on the current state of the
 * instance:
 * - if active then:
 *    - if in an undo statement => the latest undo entry (last)
 *    - if in an redo statement => the earliest redo entry (first)
 * - if inactive: the latest undo entry (last)
 *
 * @return the id of the active recordor -1
 */
qint64 ReSqliteUn::getActiveId (UndoRedoType ty) const
{
    qint64 result = -1;
    ReSqliteUn::SqLiteResult rc = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    for (;;) {
        static const char * for_undo =
                "SELECT MAX(id) FROM " RESQUN_TBL_IDX " "
                    "WHERE status=" STR(RESQUN_MARK_UNDO) ";\n";
        static const char * for_redo =
                "SELECT MIN(id) FROM " RESQUN_TBL_IDX " "
                    "WHERE status=" STR(RESQUN_MARK_REDO) ";\n";
        const char * statement = NULL;
        switch (ty) {
        case UndoType: {
            statement = for_undo;
            break; }
        case RedoType: {
            statement = for_redo;
            break; }
        case NoUndoRedo:
        case CurrentUndoRedo:
        case BothUndoRedo:
        default: {
            if (is_active_) {
                if (in_undo_) {
                    statement = for_undo;
                } else {
                    statement = for_redo;
                }
            } else {
                statement = for_undo;
            }
            break; }
        }

        rc = sqlite3_prepare_v2 (dtb_, statement, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            RESQLITEUN_DEBUGM("getActiveId(): prepare failed: %s\n",
                              sqlite3_errmsg(dtb_));
            break;
        }

        rc = sqlite3_step (stmt);
        if (rc != SQLITE_ROW) {
            RESQLITEUN_DEBUGM("getActiveId(): step failed: %s\n",
                              sqlite3_errmsg(dtb_));
            break;
        }

        result = sqlite3_column_int64 (stmt, 0);
        break;
    }
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    return result;
}
/* ========================================================================= */


/*  CLASS    =============================================================== */
//
//
//
//
/* ------------------------------------------------------------------------- */
/* ========================================================================= */

