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
#include <QStringBuilder>

/*  INCLUDES    ============================================================ */
//
//
//
//
/*  DEFINITIONS    --------------------------------------------------------- */

const char * ReSqliteUn::s_prefix_ = RESQUN_PREFIX;
const char * ReSqliteUn::s_fun_table_ = RESQUN_FUN_TABLE;
const char * ReSqliteUn::s_sql_ur_count_ =
        "SELECT "
        "(SELECT COUNT(*) FROM " RESQUN_TBL_TEMP " WHERE status=" RESQUN_MARK_UNDO ") AS UNDO,"
        "(SELECT COUNT(*) FROM " RESQUN_TBL_TEMP " WHERE status=" RESQUN_MARK_REDO ") AS REDO;";

ReSqliteUn * ReSqliteUn::instance_ = NULL;

typedef void(*xEntryPoint)(void);
xEntryPoint getEntryPoint ();

static QLatin1String comma (",");
static QString empty;

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
    db_ (db),
    is_active_ (false)
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
/**
 * The point of this method is to create an sql statement like the following:
 *
 * @code
 * CREATE TEMP TRIGGER resqun_Test_i
 *     AFTER INSERT ON Test WHEN (SELECT resqun_active())=1
 *     BEGIN UPDATE resqun_sqlite_undo
 *         SET sql=sql || 'DELETE FROM Test WHERE rowid='||NEW.rowid||';'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM resqun_sqlite_undo);
 *     END;
 * @endcode
 *
 * This creates a trigger that fires when a certain table (in this case called
 * `Test`) gets a new row by in INSERT statement. The RWOID represents
 * the internal id of the record (see http://sqlite.org/rowidtable.html).
 *
 * Once fired the trigger will write inside the `resqun_sqlite_undo` table
 * the statement that will undo current action.
 */
QString ReSqliteUn::sqlInsertTrigger (const QString & s_table)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_i \n"
        "AFTER INSERT ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
        "BEGIN UPDATE " RESQUN_TBL_TEMP " \n"
            "SET sql=sql || 'DELETE FROM ") % s_table % QStringLiteral(" WHERE rowid='||NEW.rowid||';' \n"
            "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP ");\n"
        "END;\n");
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * The point of this method is to create an sql statement like the following:
 *
 * @code
 * CREATE TEMP TRIGGER resqun_Test_d
 *     BEFORE DELETE ON Test WHEN (SELECT resqun_active())=1
 *     BEGIN UPDATE resqun_sqlite_undo
 *         SET sql=sql ||'INSERT INTO Test(rowid,id,data,data1) VALUES('||OLD.rowid||','||quote(OLD.id)||','||quote(OLD.data)||','||quote(OLD.data1)||');'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM resqun_sqlite_undo);
 *     END;
 * @endcode
 *
 * This creates a trigger that fires when a certain table (in this case called
 * `Test`) looses a row in a DELETE statement. The RWOID represents
 * the internal id of the record (see http://sqlite.org/rowidtable.html).
 *
 * Once fired the trigger will write inside the `resqun_sqlite_undo` table
 * the statement that will undo current action.
 */
QString ReSqliteUn::sqlDeleteTrigger (
        const QString &s_table, const QString & s_column_names,
        const QString & s_column_values)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_d \n"
             "BEFORE DELETE ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
             "BEGIN UPDATE " RESQUN_TBL_TEMP " \n"
                 "SET sql=sql ||'INSERT INTO ") % s_table % QStringLiteral("(rowid") % s_column_names % QStringLiteral(") "
                        "VALUES('||OLD.rowid||',") % s_column_values % QStringLiteral(");' \n"
                 "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP "); \n"
             "END;\n\n");
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * There are two strategies when dealing with an UPDATE statement:
 *
 * 1. if the data in this table gets updated on a column by column basis
 *    (there are a lot of columns and only a few of them get updated
 *    at the same time) then it is more efficient to create triggers for
 *    each individual column and coresponding undo entries (this method).
 *
 * 2. if, on the other hand, the entire row is saved every time or
 *    the fields are rather small then it is moer efficient to create a single
 *    trigger for the table and one entry for the entire row that is
 *    being updated.
 *
 * The point of this method is to create an sql statement like the following:
 *
 * @code
 * CREATE TEMP TRIGGER resqun_Test_u_data
 *     AFTER UPDATE OF data ON Test WHEN (SELECT resqun_active())=1
 *     BEGIN UPDATE resqun_sqlite_undo
 *         SET sql=sql || 'UPDATE Test SET data WHERE rowid='||OLD.rowid||';'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM resqun_sqlite_undo);
 *     END;
 * @endcode
 *
 */
QString ReSqliteUn::sqlUpdateTriggerPerColumn (
        const QString &s_table, const QString &s_column)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_u_") % s_column % QStringLiteral(" \n"
             "AFTER UPDATE OF ") % s_column % QStringLiteral(" ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
             "BEGIN UPDATE " RESQUN_TBL_TEMP " \n"
                 "SET sql=sql || 'UPDATE ") % s_table % QStringLiteral(" SET ") % s_column % QStringLiteral(" WHERE rowid='||OLD.rowid||';' \n"
                 "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP ");\n"
             "END;\n\n");
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * There are two strategies when dealing with an UPDATE statement:
 *
 * 1. if the data in this table gets updated on a column by column basis
 *    (there are a lot of columns and only a few of them get updated
 *    at the same time) then it is more efficient to create triggers for
 *    each individual column and coresponding undo entries.
 *
 * 2. if, on the other hand, the entire row is saved every time or
 *    the fields are rather small then it is moer efficient to create a single
 *    trigger for the table and one entry for the entire row that is
 *    being updated (this method).
 *
 * The point of this method is to create an sql statement like the following:
 *
 * @code
 * CREATE TEMP TRIGGER resqun_Test_u
 *     AFTER UPDATE ON Test WHEN (SELECT resqun_active())=1
 *     BEGIN UPDATE resqun_sqlite_undo
 *         SET sql=sql || 'UPDATE Test SET data='||quote(OLD.data)||',data1='||quote(OLD.data1)||' WHERE rowid='||OLD.rowid||';'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM resqun_sqlite_undo);
 *     END;
 * @endcode
 *
 */
QString ReSqliteUn::sqlUpdateTriggerPerTable (const QString &s_table, const QString & s_column_list)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_u \n"
         "AFTER UPDATE ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
         "BEGIN UPDATE " RESQUN_TBL_TEMP " \n"
             "SET sql=sql || 'UPDATE ") % s_table % QStringLiteral(" SET ") % s_column_list % QStringLiteral(" WHERE rowid='||OLD.rowid||';' \n"
             "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP "); \n"
         "END; \n\n");
}
/* ========================================================================= */


/* ------------------------------------------------------------------------- */
/**
 * We discover the structure of the table by using PRAGMA table_info(Table);
 * which returns one row per column with following columns:
 *
 *   0. cid - the id of the record;
 *   1. name - the name of the column;
 *   2. type - a string representing the type (INTEGER, TEXT, ...);
 *   3. notnull - flag that tells us if NOT NULL constraint is active (1) or not (0);
 *   4. dflt_value - default value for the column;
 *   5. pk - flag that tells us if this is a primary key or not.
 *
 *
 */
QString ReSqliteUn::sqlTriggers (
        const QString & table, UpdateBehaviour update_kind)
{
    sqlite3_stmt *stmt;

    QString statement = QStringLiteral("PRAGMA table_info(") % table %
            QStringLiteral(");\n");

    int rc = sqlite3_prepare16 (
                static_cast<sqlite3 *>(db_), statement.utf16 (),
                statement.size () * sizeof(QChar), &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }

    QString del_col_name;
    QString del_col_value;
    QString upd_col_value;
    QString upd_tbl_value;

    enum TableInfoColumns {
        col_cid = 0, // the id of the record;
        col_name, // the name of the column;
        col_type, // a string representing the type (INTEGER, TEXT, ...);
        col_notnull, // flag that tells us if NOT NULL constraint is active (1) or not (0);
        col_dflt_value, // default value for the column;
        col_pk, // flag that tells us if this is a primary key or not.
    };

    bool b_ret = true;
    for (;;) {
        // Get next record.
        rc = sqlite3_step (stmt);
        if (rc == SQLITE_DONE) {
            break;
        } else if (rc != SQLITE_ROW) {
            b_ret = false;
            break;
        }

        bool is_primary = (sqlite3_column_int (stmt, col_pk) == 1);
        QString name = QString (reinterpret_cast<const QChar *>(
                    sqlite3_column_text16(stmt, col_name)),
                    sqlite3_column_bytes16(stmt, col_name) / sizeof(QChar));

        // This results in column1,column2,column3
        if (!del_col_name.isEmpty ()) {
            del_col_name.append (comma);
        }
        del_col_name.append (name);

        // This results in column1,column2,column3
//        del_col_value.append (comma);
//        QString quoted_old_colum = QStringLiteral("'||quote(OLD.") % name %
//                QStringLiteral(")||'");
//        del_col_value.append (quoted_old_colum);

        del_col_value.append (
                    comma % QStringLiteral("'||quote(OLD.") % name %
                    QStringLiteral(")||'"));

        // Primary keys excluded from those that trigger an unco step.
        if (is_primary)
            continue;

        switch (update_kind) {
        case OneTriggerPerUpdatedColumn: {
            upd_col_value.append (sqlUpdateTriggerPerColumn (table, name));
            break; }
        case OneTriggerPerUpdatedTable: {
            if (!upd_tbl_value.isEmpty ()) {
                upd_tbl_value.append (comma);
            }
            upd_tbl_value.append (
                        name % QStringLiteral("='||quote(OLD.") % name %
                        QStringLiteral(")||'"));
            break; }
        case NoTriggerForUpdate: {
            break; }
        }
    }

    if (rc == SQLITE_DONE) {
        return
            (update_kind == OneTriggerPerUpdatedTable ?
                 sqlUpdateTriggerPerTable (table, upd_tbl_value) :
                 empty) %
            sqlDeleteTrigger (table, del_col_name, del_col_value) %
            sqlInsertTrigger (table) %
            upd_col_value;
    } else {
        return empty;
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * We're adding a table to the list of tables managed by the
 * undo-redo mechanism.
 */
bool ReSqliteUn::attachToTable (
        void * ctx, const QString & table, UpdateBehaviour update_kind)
{
    RESQLITEUN_TRACE_ENTRY;
    bool b_ret = false;
    for (;;) {
        sqlite3_context *context = static_cast<sqlite3_context *>(ctx);
        QString statements = sqlTriggers (table, update_kind);
        printf(statements.toLatin1().constData());

        // This is inefficient as toUtf8 will allocate a new buffer;
        // as sqlite3_exec only works with Utf8 (there is no 16 alternative)
        // we are left with three options: use utf8 all over the place,
        // the one below or reimplementing sqlite3_exec.
        // As the this function will probably used only when the program starts,
        // once for each table, the performance penality is neglijable.
        int rc = sqlite3_exec (
                    static_cast<sqlite3 *>(db_),
                    statements.toUtf8().constData (),
                    NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_result_error_code (context, rc);
        }

        b_ret = true;
        break;
    }
    RESQLITEUN_TRACE_EXIT;
    return b_ret;
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
int ReSqliteUn::count (qint64 &undo_entries, qint64 &redo_entries)
{
    int rc = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    for (;;) {
        int rc = sqlite3_prepare_v2 (
                    static_cast<sqlite3 *>(db_),
                    s_sql_ur_count_, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {

            break;
        }

        rc = sqlite3_step (stmt);
        if (rc != SQLITE_ROW) {

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
