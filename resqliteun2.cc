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
/**
 * The point of this method is to create an sql statement like the following:
 *
 * @code
 * CREATE TEMP TRIGGER _u_Test_i
 *     AFTER INSERT ON Test WHEN (SELECT undoable_active())=1
 *     BEGIN UPDATE _sqlite_undo
 *         SET sql=sql || 'DELETE FROM Test WHERE rowid='||NEW.rowid||';'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM _sqlite_undo);
 *     END;
 * @endcode
 *
 * This creates a trigger that fires when a certain table (in this case called
 * `Test`) gets a new row by in INSERT statement. The RWOID represents
 * the internal id of the record (see http://sqlite.org/rowidtable.html).
 *
 * Once fired the trigger will write inside the `_sqlite_undo` table
 * the statement that will undo current action.
 */
QString ReSqliteUn::sqlInsertTrigger (const QString & s_table)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_i "
        "AFTER INSERT ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 "
        "BEGIN UPDATE " RESQUN_TBL_TEMP " "
            "SET sql=sql || 'DELETE FROM ") % s_table % QStringLiteral(" WHERE rowid='||NEW.rowid||';' "
            "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP ");"
                                                                   "END;");
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * The point of this method is to create an sql statement like the following:
 *
 * @code
 * CREATE TEMP TRIGGER _u_Test_d
 *     BEFORE DELETE ON Test WHEN (SELECT undoable_active())=1
 *     BEGIN UPDATE _sqlite_undo
 *         SET sql=sql ||'INSERT INTO Test(rowid,id,data,data1) VALUES('||OLD.rowid||','||quote(OLD.id)||','||quote(OLD.data)||','||quote(OLD.data1)||');'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM _sqlite_undo);
 *     END;
 * @endcode
 *
 * This creates a trigger that fires when a certain table (in this case called
 * `Test`) looses a row in a DELETE statement. The RWOID represents
 * the internal id of the record (see http://sqlite.org/rowidtable.html).
 *
 * Once fired the trigger will write inside the `_sqlite_undo` table
 * the statement that will undo current action.
 */
QString ReSqliteUn::sqlDeleteTrigger (
        const QString &s_table, const QString & s_column_list)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_d "
             "BEFORE DELETE ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 "
             "BEGIN UPDATE " RESQUN_TBL_TEMP " "
                 "SET sql=sql ||'INSERT INTO ") % s_table % QStringLiteral("(rowid,id,data,data1) "
                        "VALUES(") % s_column_list % QStringLiteral(");' "
                 "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP "); "
             "END;");
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
 * CREATE TEMP TRIGGER _u_Test_u_data
 *     AFTER UPDATE OF data ON Test WHEN (SELECT undoable_active())=1
 *     BEGIN UPDATE _sqlite_undo
 *         SET sql=sql || 'UPDATE Test SET data WHERE rowid='||OLD.rowid||';'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM _sqlite_undo);
 *     END;
 * @endcode
 *
 */
QString ReSqliteUn::sqlUpdateTriggerPerColumn (
        const QString &s_table, const QString &s_column)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_u_") % s_column % QStringLiteral(" "
             "AFTER UPDATE OF ") % s_column % QStringLiteral(" ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 "
             "BEGIN UPDATE " RESQUN_TBL_TEMP " "
                 "SET sql=sql || 'UPDATE ") % s_table % QStringLiteral(" SET ") % s_column % QStringLiteral(" WHERE rowid='||OLD.rowid||';' "
                 "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP ");"
             "END;");
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
 * CREATE TEMP TRIGGER _u_Test_u
 *     AFTER UPDATE ON Test WHEN (SELECT undoable_active())=1
 *     BEGIN UPDATE _sqlite_undo
 *         SET sql=sql || 'UPDATE Test SET data='||quote(OLD.data)||',data1='||quote(OLD.data1)||' WHERE rowid='||OLD.rowid||';'
 *         WHERE ROWID=(SELECT MAX(ROWID) FROM _sqlite_undo);
 *     END;
 * @endcode
 *
 */
QString ReSqliteUn::sqlUpdateTriggerPerTable (const QString &s_table, const QString & s_column_list)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_u "
         "AFTER UPDATE ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 "
         "BEGIN UPDATE " RESQUN_TBL_TEMP " "
             "SET sql=sql || 'UPDATE ") % s_table % QStringLiteral(" SET ") % s_column_list % QStringLiteral(" WHERE rowid='||OLD.rowid||';' "
             "WHERE ROWID=(SELECT MAX(ROWID) FROM " RESQUN_TBL_TEMP "); "
         "END; ");
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * We're adding a table to the list of tables managed by the
 * undo-redo mechanism.
 */
bool ReSqliteUn::attachToTable (const QString & table, int)
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
