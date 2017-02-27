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
            break;
        }
        if ((sqlite3_value_type(argv[1]) != SQLITE_INTEGER)) {
            sqlite3_result_error (context,
                                  "Second argument to " RESQUN_FUN_TABLE " must be an integer", -1);
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
            break;
        }

        QString table = value2string(argv[0]);
        if (table.length() == 0) {
            sqlite3_result_error (
                        context,
                        "First argument to " RESQUN_FUN_TABLE
                        " must be a non-empty string", -1);
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

    p_app->attachToTable ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `begin` function.
static void epoint_begin (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
    assert(p_app != NULL);

    p_app->attachToTable ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `end` function.
static void epoint_end (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
    assert(p_app != NULL);

    p_app->attachToTable ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `undo` function.
static void epoint_undo (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
    assert(p_app != NULL);

    p_app->attachToTable ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `redo` function.
static void epoint_redo (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
    assert(p_app != NULL);

    p_app->attachToTable ();
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
