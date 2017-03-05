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

#include <sqlite/sqlite3ext.h>
SQLITE_EXTENSION_INIT1

#include "resqliteun.h"
#include "resqliteun-private.h"

#include <assert.h>
#include <QString>

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



extern "C" {

/* ------------------------------------------------------------------------- */
//! Implementation of the `table` function.
static void epoint_table (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    RESQLITEUN_TRACE_ENTRY;
    bool b_ret = false;
    ReSqliteUn::SqLiteResult rc;
    for (;;) {
        ReSqliteUn * p_app = static_cast<ReSqliteUn *>(
                    sqlite3_user_data (context));
        assert(p_app != NULL);
        sqlite3 * db = sqlite3_context_db_handle(context);
        assert(db == static_cast<sqlite3 *>(p_app->db_));


        // Check arguments types.
        if ((sqlite3_value_type(argv[0]) != SQLITE_TEXT)) {
            sqlite3_result_error (
                        context,
                        "First argument to " RESQUN_FUN_TABLE
                        " must be a string", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            break;
        }
        if ((sqlite3_value_type(argv[1]) != SQLITE_INTEGER)) {
            sqlite3_result_error (
                        context,
                        "Second argument to " RESQUN_FUN_TABLE
                        " must be an integer", -1);
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

        // Get the name of the table.
        QString table = ReSqliteUn::value2string (argv[0]);
        if (table.length() == 0) {
            sqlite3_result_error (
                        context,
                        "First argument to " RESQUN_FUN_TABLE
                        " must be a non-empty string", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            break;
        }

        // Actually attaching to the table goes on inside here.
        rc = p_app->attachToTable (
                    table,
                    static_cast<ReSqliteUn::UpdateBehaviour>(update_type));
        if (rc != SQLITE_OK) {
            sqlite3_result_error (context, RESQUN_FUN_TABLE "failed", -1);
            sqlite3_result_error_code (context, rc);
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
    RESQLITEUN_TRACE_ENTRY;
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
    assert(p_app != NULL);

    sqlite3_result_int (context, p_app->is_active_ ? 1 : 0);
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `begin` function.
static void epoint_begin (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    RESQLITEUN_TRACE_ENTRY;
    for (;;) {
        QString name;
        if (argc > 1) {
            sqlite3_result_error (
                        context,
                        RESQUN_FUN_BEGIN " takes zero or one argument", -1);
            sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
            return;
        } else if (argc == 1) {
            name = ReSqliteUn::value2string (argv[0]);
        }

        ReSqliteUn * p_app = static_cast<ReSqliteUn *>(sqlite3_user_data (context));
        assert(p_app != NULL);

        if (p_app->is_active_) {
            sqlite3_result_error(context, "Already in an update", -1);
            break;
        }

        sqlite3 * db = sqlite3_context_db_handle(context);
        assert(db == static_cast<sqlite3 *>(p_app->db_));

        ReSqliteUn::SqLiteResult rc = p_app->begin (name);
        if (rc != SQLITE_OK) {
            sqlite3_result_error_code (context, rc);
            break;
        }

        break;
    }
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the application's `getid` function.
static void epoint_getid (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    RESQLITEUN_TRACE_ENTRY;

    ReSqliteUn * p_app =
            static_cast<ReSqliteUn *>(sqlite3_user_data (context));
    assert(p_app != NULL);

    sqlite3 * db = sqlite3_context_db_handle(context);
    assert(db == static_cast<sqlite3 *>(p_app->db_));

    sqlite3_result_int64 (context, p_app->getActiveId ());

    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `end` function.
#define STR_END_USAGE \
            "0 (for no output), "\
            "1 (to return the number of undo entries as an integer), "\
            "2 (to return the number of redo entries as an integer), "\
            "3 (to return an array as a blob with undo and redo count), "\
            "or 4 (to print the undo and redo as text"
static void epoint_end (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    RESQLITEUN_TRACE_ENTRY;
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

        int rc = p_app->end ();
        if (rc != SQLITE_OK) {
            sqlite3_result_error(context, "Not in an update", -1);
            break;
        }

        sqlite3 * db = sqlite3_context_db_handle(context);
        assert(db == static_cast<sqlite3 *>(p_app->db_));

        qint64 entries[2];
        rc = p_app->count (entries[0], entries[1]);
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
            if ((sqlite3_value_type(argv[0]) != SQLITE_INTEGER)) {
                sqlite3_result_error (context,
                                      "First argument to " RESQUN_FUN_END
                                      " must be an integer "
                                      STR_END_USAGE, -1);
                sqlite3_result_error_code (context, SQLITE_CONSTRAINT);
                return;
            }
            EndOutputType ty = static_cast<EndOutputType>(
                        sqlite3_value_int(argv[0]));
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
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */


/* ------------------------------------------------------------------------- */
//! Implementation of the `redo` and `undo` function.
static void preform_ur (sqlite3_context *context, bool for_undo)
{
    RESQLITEUN_TRACE_ENTRY;
    int rc = SQLITE_OK;

    for (;;) {

        ReSqliteUn * p_app = static_cast<ReSqliteUn *>(
                    sqlite3_user_data (context));
        assert(p_app != NULL);
        sqlite3 * db = sqlite3_context_db_handle(context);
        assert(db == static_cast<sqlite3 *>(p_app->db_));

        if (p_app->is_active_) {
            sqlite3_result_error(
                        context,
                        "In an update (forgot to call " RESQUN_FUN_END "?)", -1);
            rc = SQLITE_MISUSE;
            break;
        }

        QString s_error;
        rc = p_app->performUndoRedo (for_undo, s_error);
        if (rc != SQLITE_OK) {
            if (!s_error.isEmpty()) {
                sqlite3_result_error(context, s_error.toUtf8 ().constData (), -1);
            }
        }

        break;
    }
    if (rc != SQLITE_OK) {
        sqlite3_result_error_code (context, rc);
    }
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `undo` function.
static void epoint_undo (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    RESQLITEUN_TRACE_ENTRY;
    preform_ur (context, true);
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the `redo` function.
static void epoint_redo (
            sqlite3_context *context, int argc, sqlite3_value **argv)
{
    RESQLITEUN_TRACE_ENTRY;
    preform_ur (context, false);
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Implementation of the application's `destroy` function.
static void epoint_destroy (void *value)
{
    RESQLITEUN_TRACE_ENTRY;
    ReSqliteUn * p_app = static_cast<ReSqliteUn *>(value);
    delete p_app;
    RESQLITEUN_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! The entry point used by sqlite.
RESQLITEUN_EXPORT int sqlite3_resqliteun_init (
            sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    RESQLITEUN_TRACE_ENTRY;
    int rc = SQLITE_ERROR;
    for (;;) {
        SQLITE_EXTENSION_INIT2(pApi);

        QString s_error;
        ReSqliteUn * p_app = ReSqliteUn::create (db, s_error);
        if (p_app == NULL) {
            *pzErrMsg = sqlite3_mprintf ("%s\n", s_error.toUtf8().constData ());
            break;
        }

        rc = SQLITE_OK;
        break;
    }
    RESQLITEUN_TRACE_EXIT;
    return rc;
}
/* ========================================================================= */

} // extern "C"

/* ------------------------------------------------------------------------- */
ReSqliteUnManager::xEntryPoint ReSqliteUnManager::getEntryPoint ()
{
    return (void(*)(void))sqlite3_resqliteun_init;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
typedef void (*sqliteEntryPoint)(sqlite3_context*,int,sqlite3_value**);

//! Compact the code by defining the entrypoints in an array of
//! structures liek this one.
struct FuncDescr {

    //! Name of the function
    const char * name_;
    //! number of arguements
    int arg_count_;
    //! address of the callback.
    sqliteEntryPoint function_;
    //! do we destroy the ReSqliteUn when the database closes?
    bool has_destroy_;

    FuncDescr(
            const char * name, int arg_count,
            sqliteEntryPoint function,
            bool has_destroy) :
        name_(name),
        arg_count_ (arg_count),
        function_ (function),
        has_destroy_ (has_destroy)
    {}
};

#define HAS_VAR_ARG -1
#define NO_ARG 0

FuncDescr entry_points[] = {
    {RESQUN_FUN_TABLE,  2,              epoint_table,   true},
    {RESQUN_FUN_ACTIVE, NO_ARG,         epoint_active,  false},
    {RESQUN_FUN_BEGIN,  HAS_VAR_ARG,    epoint_begin,   false},
    {RESQUN_FUN_END,    HAS_VAR_ARG,    epoint_end,     false},
    {RESQUN_FUN_UNDO,   NO_ARG,         epoint_undo,    false},
    {RESQUN_FUN_REDO,   NO_ARG,         epoint_redo,    false},
    {RESQUN_FUN_GETID,  NO_ARG,         epoint_getid,   false}
};
#define entry_point_count sizeof(entry_points) / sizeof(entry_points[0])
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ReSqliteUn *ReSqliteUnManager::create (void *database, QString & s_error)
{
    RESQLITEUN_TRACE_ENTRY;

    sqlite3* db = static_cast<sqlite3*>(database);
    ReSqliteUn * p_app = new ReSqliteUn (static_cast<void*> (db));

    int rc = SQLITE_OK;
    for (;;) {
        // AUTOINCREMENT is justified because we use the indices to
        // have the entries sorted by time.
        rc = sqlite3_exec (db,

            // This is where each undo or redo entry is stored.
            "CREATE TEMP TABLE IF NOT EXISTS " RESQUN_TBL_IDX "("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT, "
                "status INTEGER "
            ");"

            // This is where the data for each individual step is stored
            // An undo or redo method may have zero or more
            // individual steps associated with them.
            "CREATE TEMP TABLE IF NOT EXISTS " RESQUN_TBL_TEMP "("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "sql TEXT, "
                "idxid INTEGER, "
                "FOREIGN KEY(idxid) REFERENCES " RESQUN_TBL_IDX "(id) "
            ");"

            // We're creating an index here because
            // `SELECT id FROM sqlite_undo WHERE idxid=XX` is common.
            "CREATE INDEX IF NOT EXISTS " RESQUN_INDEX_DATA " "
                "ON " RESQUN_TBL_TEMP "(idxid);" ,



            NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            s_error = tr(
                        "Failed to create temporary tables in ReSqliteUn `"
                        RESQUN_FUN_TABLE "`: %s")
                    .arg (sqlite3_errmsg(db));
            break;
        }

        for (int i = 0; i < entry_point_count; ++i) {
            FuncDescr * fd = &entry_points[i];
            rc = sqlite3_create_function_v2 (
                        db,
                        /* zFunctionName */ fd->name_,
                        /* nArg */ fd->arg_count_,
                        /* eTextRep */ SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                        /* pApp */ static_cast<void*>(p_app),
                        /* xFunc */ fd->function_,
                        /* xStep */ NULL,
                        /* xFinal */ NULL,
                        /* xDestroy */ fd->has_destroy_ ? epoint_destroy : NULL);
            if (rc != SQLITE_OK) {
                s_error = tr (
                            "Failed to register function `%1`: %2")
                        .arg (fd->name_)
                        .arg (sqlite3_errmsg(db));
                break;
            }
        }
        if (rc != SQLITE_OK)
            break;


        break;
    }
    if (rc != SQLITE_OK) {
        delete p_app;
        p_app = NULL;
    }

    RESQLITEUN_TRACE_EXIT;
    return p_app;
}
/* ========================================================================= */


/*  SQLITE Entry Points   ================================================== */
//
//
//
//
/* ------------------------------------------------------------------------- */
/* ========================================================================= */
