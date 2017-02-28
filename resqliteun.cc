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

#include <sqlite/sqlite3ext.h>
SQLITE_EXTENSION_INIT1

#include "resqliteun.h"
#include "resqliteun-private.h"

#include <assert.h>

/*  INCLUDES    ============================================================ */
//
//
//
//
/*  DEFINITIONS    --------------------------------------------------------- */

/*  DEFINITIONS    ========================================================= */
//
//
//
//
/*  SQLITE Entry Points   -------------------------------------------------- */

/* ------------------------------------------------------------------------- */
//! Helper for retrieving strings from values.
static QString value2string (sqlite3_value * value)
{
    return QString(reinterpret_cast<const QChar *>(
                       sqlite3_value_text16 (value)),
                       sqlite3_value_bytes16 (value) / sizeof(QChar));
}
/* ========================================================================= */



extern "C" {



/* ------------------------------------------------------------------------- */
//! Implementation of the `table` function.
static void epoint_table (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    RESQLITEUN_TRACE_ENTRY;
    bool b_ret = false;
    for (;;) {

        ReSqliteUn * p_app = static_cast<ReSqliteUn *>(
                    sqlite3_user_data (context));
        assert(p_app != NULL);

        // Check arguments types.
        if ((sqlite3_value_type(argv[0]) != SQLITE_TEXT)) {
            sqlite3_result_error (context,
                                  "First argument to " RESQUN_FUN_TABLE " must be a sting", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            break;
        }
        if ((sqlite3_value_type(argv[1]) != SQLITE_INTEGER)) {
            sqlite3_result_error (context,
                                  "Second argument to " RESQUN_FUN_TABLE " must be an integer", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            return;
        }

        // Check arguments values.
        int update_type = sqlite3_value_int(argv[1]);
        if (
                (update_type != ReSqliteUn::NoTriggerForUpdate) &&
                (update_type != ReSqliteUn::OneTriggerPerUpdatedTable) &&
                (update_type != ReSqliteUn::OneTriggerPerUpdatedColumn)) {

            sqlite3_result_error (
                        context,
                        "Second argument to " RESQUN_FUN_TABLE
                        " must be 0, 1 or 2", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            break;
        }

        QString table = value2string(argv[0]);
        if (table.length() == 0) {
            sqlite3_result_error (
                        context,
                        "First argument to " RESQUN_FUN_TABLE
                        " must be a non-empty string", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            break;
        }

        sqlite3 * db = sqlite3_context_db_handle(context);
        assert(db == static_cast<sqlite3 *>(p_app->db_));

        if (!p_app->attachToTable (
                    context, table,
                    static_cast<ReSqliteUn::UpdateBehaviour>(update_type))) {

            sqlite3_result_error (context, RESQUN_FUN_TABLE "failed", -1);
            break;
        }

        b_ret = true;
        break;
    }
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `active` function.
static void epoint_active (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
    assert(p_app != NULL);

    sqlite3_result_int (context, p_app->is_active_);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `begin` function.
static void epoint_begin (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    for (;;) {
        ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
        assert(p_app != NULL);

        if (p_app->is_active_) {
            sqlite3_result_error(context, "Already in an update", -1);
            break;
        }

        sqlite3 * db = sqlite3_context_db_handle(context);
        assert(db == static_cast<sqlite3 *>(p_app->db_));

        // Remove all "redo" entries and place a marker for first undo entry.
        int rc = sqlite3_exec(
            db,
            "SAVEPOINT " RESQUN_SVP_BEGIN ";"
            "DELETE FROM " RESQUN_TBL_TEMP " WHERE status=" RESQUN_MARK_REDO ";"
            "INSERT INTO " RESQUN_TBL_TEMP "(sql, status) VALUES(''," RESQUN_MARK_UNDO ");",
            NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_exec (db,
                "ROLLBACK TO SAVEPOINT " RESQUN_SVP_BEGIN,
                NULL, NULL, NULL);
            sqlite3_result_error_code (context, rc);
        } else {
            p_app->is_active_ = true;
        }
        sqlite3_exec (db,"RELEASE SAVEPOINT " RESQUN_SVP_BEGIN,
                NULL, NULL, NULL);

        break;
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `end` function.
#define STR_END_USAGE \
            "0 (for no output), "\
            "1 (to return the number of undo enttries as an integer), "\
            "2 (to return the number of redo enttries as an integer), "\
            "3 (to return an array as a blob with undo and redo count), "\
            "or 4 (to print the undo and redo as text"
static void epoint_end (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    sqlite3_stmt *stmt = NULL;
    for (;;) {
        if (argc > 1) {
            sqlite3_result_error (
                        context,
                        RESQUN_FUN_END " takes zero or one argument", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            return;
        }

        ReSqliteUn * p_app =
                static_cast<ReSqliteUn *>(sqlite3_user_data (context));
        assert(p_app != NULL);

        if (!p_app->is_active_) {
            sqlite3_result_error(context, "Not in an update", -1);
            break;
        }

        sqlite3 * db = sqlite3_context_db_handle(context);
        assert(db == static_cast<sqlite3 *>(p_app->db_));

        qint64 entries[2];
        int rc = p_app->count (entries[0], entries[1]);
        if (rc != SQLITE_OK) {
            sqlite3_result_error(
                        context,
                        "Failed to retrieve values from temporary table",
                        -1);
            sqlite3_result_error_code (context, rc);
            break;
        }


        enum EndOutputType {
            NoOutput = 0,
            UndoCountAsInt = 1,
            RedoCountAsInt = 2,
            BothAs64bitArray = 3,
            BothAsText = 4,

            EndOutputTypeMax
        };
        EndOutputType ty = BothAsText;

        if (argc > 0) {
            if ((sqlite3_value_type(argv[1]) != SQLITE_INTEGER)) {
                sqlite3_result_error (context,
                                      "First argument to " RESQUN_FUN_END
                                      " must be an integer "
                                      STR_END_USAGE, -1);
                sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
                return;
            }
            EndOutputType ty = static_cast<EndOutputType>(
                        sqlite3_value_int(argv[1]));
            if ((ty < 0) || (ty >= EndOutputTypeMax)) {
                sqlite3_result_error (context,
                                      "First argument to " RESQUN_FUN_END " must be "
                                      STR_END_USAGE, -1);
                sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
                return;
            }

        }

        switch (ty) {
        case NoOutput: {
            break; }
        case UndoCountAsInt: {
            sqlite3_result_int64 (context, entries[0]);
            break; }
        case RedoCountAsInt: {
            sqlite3_result_int64 (context, entries[1]);
            break; }
        case BothAs64bitArray: {
            sqlite3_result_blob(context, &entries, sizeof(entries), SQLITE_TRANSIENT);
            break; }
        case BothAsText: {
            char *result;
            result = sqlite3_mprintf (
                        "UNDO=%lld\nREDO=%lld",
                        entries[0], entries[1]);
            sqlite3_result_text (context, result, -1, sqlite3_free);
            break; }
        }

        p_app->is_active_ = false;
        break;
    }

    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
}
/* ========================================================================= */


/* ------------------------------------------------------------------------- */
//! Implementation of the `redo` and `undo` function.
static void preform_ur (sqlite3_context *context, bool for_undo)
{
    int rc = SQLITE_OK;
    bool rollback = false;

    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(
                sqlite3_user_data (context));
    assert(p_app != NULL);
    sqlite3 * db = sqlite3_context_db_handle(context);
    assert(db == static_cast<sqlite3 *>(p_app->db_));

    for (;;) {

        if (p_app->is_active_) {
            sqlite3_result_error(
                        context,
                        "In an update (forgot to call " RESQUN_FUN_END "?)", -1);
            rc = SQLITE_MISUSE;
            break;
        }

        sqlite_int64 maxid;
        rc = p_app->lastStepId (for_undo, maxid);
        if (rc != SQLITE_OK) {
            break;
        }
        if (maxid == 0) {
            sqlite3_result_null (context);
            break;
        }

        QString sql;
        rc = p_app->getSqlStatementForId (maxid, sql);
        if (rc != SQLITE_OK) {
            break;
        }

        if (sql.isEmpty ()) {
            sqlite3_result_null(context);
            break;
        }

        rc = sqlite3_exec(db, "SAVEPOINT " RESQUN_SVP_UNDO,
                NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            break;
        }
        rollback = true;

        rc = p_app->deleteForId (maxid);
        if (rc != SQLITE_OK) {
            break;
        }

        rc = p_app->insertNew (!for_undo);
        if (rc != SQLITE_OK) {
            break;
        }

        p_app->is_active_ = true;
        rc = sqlite3_exec (db, sql.toUtf8().constData(), NULL, NULL, NULL);
        p_app->is_active_ = false;
        if (rc != SQLITE_OK) {
            break;
        }

        qint64 entries[2];
        int rc = p_app->count (entries[0], entries[1]);
        if (rc != SQLITE_OK) {
            sqlite3_result_error(
                        context,
                        "Failed to retrieve values from temporary table",
                        -1);
            break;
        }

        sqlite3_result_blob(context, &entries, sizeof(entries), SQLITE_TRANSIENT);

//        char *result;
//        result = sqlite3_mprintf (
//                    "UNDO=%lld\nREDO=%lld",
//                    entries[0], entries[1]);
//        sqlite3_result_text (context, result, -1, sqlite3_free);


        rollback = false;
        sqlite3_exec (db, "RELEASE SAVEPOINT " RESQUN_SVP_UNDO,
            NULL, NULL, NULL);

        break;
    }
    if (rollback) {
        sqlite3_exec(db,
            "ROLLBACK TO SAVEPOINT " RESQUN_SVP_UNDO ";"
            "RELEASE SAVEPOINT " RESQUN_SVP_UNDO,
            NULL, NULL, NULL);
    }
    if (rc != SQLITE_OK) {
        sqlite3_result_error_code (context, rc);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `undo` function.
static void epoint_undo (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    preform_ur (context, true);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `redo` function.
static void epoint_redo (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    preform_ur (context, false);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the application's `destroy` function.
static void epoint_destroy (void *value)
{
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(value);
    delete p_app;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! The entry point used by sqlite.
RESQLITEUN_EXPORT int sqlite3_resqliteun_init (
            sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    int rc = SQLITE_OK;
    for (;;) {
        SQLITE_EXTENSION_INIT2(pApi);

        ReSqliteUn * p_app = new ReSqliteUn (static_cast<void*> (db));

        rc = sqlite3_exec (db,
            "CREATE TEMP TABLE " RESQUN_TBL_TEMP "(sql TEXT, status INTEGER)",
            NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            return rc;
        }

        rc = sqlite3_create_function_v2 (
                    db,
                    /* zFunctionName */ RESQUN_FUN_TABLE,
                    /* nArg */ 2,
                    /* eTextRep */ SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                    /* pApp */ static_cast<void*>(p_app),
                    /* xFunc */ epoint_table,
                    /* xStep */ NULL,
                    /* xFinal */ NULL,
                    /* xDestroy */ epoint_destroy);
        if (rc != SQLITE_OK) {
            *pzErrMsg = sqlite3_mprintf ("Failed to register function `"
                                         RESQUN_FUN_TABLE "`: %s\n",
                                         sqlite3_errmsg(db));
            break;
        }

        rc = sqlite3_create_function_v2 (
                    db,
                    /* zFunctionName */ RESQUN_FUN_ACTIVE,
                    /* nArg */ 0,
                    /* eTextRep */ SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                    /* pApp */ static_cast<void*>(p_app),
                    /* xFunc */ epoint_active,
                    /* xStep */ NULL,
                    /* xFinal */ NULL,
                    /* xDestroy */ NULL);
        if (rc != SQLITE_OK) {
            *pzErrMsg = sqlite3_mprintf ("Failed to register function `"
                                         RESQUN_FUN_ACTIVE "`: %s\n",
                                         sqlite3_errmsg(db));
            break;
        }

        rc = sqlite3_create_function_v2 (
                    db,
                    /* zFunctionName */ RESQUN_FUN_BEGIN,
                    /* nArg */ 0,
                    /* eTextRep */ SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                    /* pApp */ static_cast<void*>(p_app),
                    /* xFunc */ epoint_begin,
                    /* xStep */ NULL,
                    /* xFinal */ NULL,
                    /* xDestroy */ NULL);
        if (rc != SQLITE_OK) {
            *pzErrMsg = sqlite3_mprintf ("Failed to register function `"
                                         RESQUN_FUN_BEGIN "`: %s\n",
                                         sqlite3_errmsg(db));
            break;
        }

        rc = sqlite3_create_function_v2 (
                    db,
                    /* zFunctionName */ RESQUN_FUN_END,
                    /* nArg */ 0,
                    /* eTextRep */ SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                    /* pApp */ static_cast<void*>(p_app),
                    /* xFunc */ epoint_end,
                    /* xStep */ NULL,
                    /* xFinal */ NULL,
                    /* xDestroy */ NULL);
        if (rc != SQLITE_OK) {
            *pzErrMsg = sqlite3_mprintf ("Failed to register function `"
                                         RESQUN_FUN_END "`: %s\n",
                                         sqlite3_errmsg(db));
            break;
        }

        rc = sqlite3_create_function_v2 (
                    db,
                    /* zFunctionName */ RESQUN_FUN_UNDO,
                    /* nArg */ 0,
                    /* eTextRep */ SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                    /* pApp */ static_cast<void*>(p_app),
                    /* xFunc */ epoint_undo,
                    /* xStep */ NULL,
                    /* xFinal */ NULL,
                    /* xDestroy */ NULL);
        if (rc != SQLITE_OK) {
            *pzErrMsg = sqlite3_mprintf ("Failed to register function `"
                                         RESQUN_FUN_UNDO "`: %s\n",
                                         sqlite3_errmsg(db));
            break;
        }

        rc = sqlite3_create_function_v2 (
                    db,
                    /* zFunctionName */ RESQUN_FUN_REDO,
                    /* nArg */ 0,
                    /* eTextRep */ SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                    /* pApp */ static_cast<void*>(p_app),
                    /* xFunc */ epoint_redo,
                    /* xStep */ NULL,
                    /* xFinal */ NULL,
                    /* xDestroy */ NULL);
        if (rc != SQLITE_OK) {
            *pzErrMsg = sqlite3_mprintf ("Failed to register function `"
                                         RESQUN_FUN_REDO "`: %s\n",
                                         sqlite3_errmsg(db));
            break;
        }

        break;
    }
    return rc;
}
/* ========================================================================= */

} // extern "C"


/* ------------------------------------------------------------------------- */
typedef void(*xEntryPoint)(void);

xEntryPoint getEntryPoint ()
{
    return (void(*)(void))sqlite3_resqliteun_init;
}
/* ========================================================================= */

/*  SQLITE Entry Points   ================================================== */
//
//
//
//
/* ------------------------------------------------------------------------- */
/* ========================================================================= */
