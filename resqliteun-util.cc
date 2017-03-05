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

#define dtb_ static_cast<sqlite3 *>(db)

static QLatin1String comma (",");
static QString empty;


/*  DEFINITIONS    ========================================================= */
//
//
//
//
/*  CLASS    --------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
QString ReSqliteUnUtil::columnText (void *statement, int column)
{
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(statement);
    return QString(reinterpret_cast<const QChar *>(
                       sqlite3_column_text16 (stmt, column)),
                       sqlite3_column_bytes16 (stmt, column) / sizeof(QChar));
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
int ReSqliteUnUtil::bind (
        void * statement, int column, const QString & s_value)
{
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(statement);
    return sqlite3_bind_text(
                stmt, column, s_value.toUtf8().constData(),
                -1, SQLITE_TRANSIENT);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
//! Helper for retrieving strings from values.
QString ReSqliteUnUtil::value2string (void * val)
{
    sqlite3_value *value = static_cast<sqlite3_value *>(val);

    return QString(reinterpret_cast<const QChar *>(
                       sqlite3_value_text16 (value)),
                       sqlite3_value_bytes16 (value) / sizeof(QChar));
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
QString ReSqliteUnUtil::sqlTriggers (
        void * db, const QString & table, UpdateBehaviour update_kind)
{
    RESQLITEUN_TRACE_ENTRY;
    sqlite3_stmt *stmt;

    QString statement = QStringLiteral("PRAGMA table_info(") % table %
            QStringLiteral(");\n");

    int rc = sqlite3_prepare16 (
                dtb_, statement.utf16 (),
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
        QString name = columnText (stmt, col_name);

        // This results in column1,column2,column3
        if (!del_col_name.isEmpty ()) {
            del_col_name.append (comma);
        }
        del_col_name.append (name);

        del_col_value.append (
                    comma % QStringLiteral("'||quote(OLD.") % name %
                    QStringLiteral(")||'"));

        // Primary keys excluded from those that trigger an undo step.
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

    QString result;
    if (rc == SQLITE_DONE) {
        result =
            (update_kind == OneTriggerPerUpdatedTable ?
                 sqlUpdateTriggerPerTable (table, upd_tbl_value) :
                 empty) %
            sqlDeleteTrigger (table, del_col_name, del_col_value) %
            sqlInsertTrigger (table) %
            upd_col_value;
    } else {
        result = empty;
    }

    RESQLITEUN_TRACE_EXIT;
    return result;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 * The point of this method is to create an sql statement like the following:
 *
 * @code
 * CREATE TEMP TRIGGER resqun_Test_i
 *     AFTER INSERT ON Test WHEN (SELECT resqun_active())=1
 *     BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (
 *            'DELETE FROM Test WHERE rowid='||NEW.rowid||';',
 *            resqun_getid()
 *         );
 *     END;
 * @endcode
 *
 * This creates a trigger that fires when a certain table (in this case called
 * `Test`) gets a new row from INSERT statement. The RWOID represents
 * the internal id of the record (see http://sqlite.org/rowidtable.html).
 *
 * Once fired the trigger will write inside the `resqun_sqlite_undo` table
 * the statement that will undo current action.
 */
QString ReSqliteUnUtil::sqlInsertTrigger (const QString & s_table)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_i \n"
        "AFTER INSERT ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
        "BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (\n"
                "'DELETE FROM ") % s_table % QStringLiteral(" WHERE rowid='||NEW.rowid||';',\n"
                RESQUN_FUN_GETID "()\n"
            ");\n"
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
 *     BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (
 *             'INSERT INTO Test(rowid,id,data,data1) VALUES('||OLD.rowid||','||quote(OLD.id)||','||quote(OLD.data)||','||quote(OLD.data1)||');',
 *            resqun_getid()
 *         );
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
QString ReSqliteUnUtil::sqlDeleteTrigger (
        const QString &s_table, const QString & s_column_names,
        const QString & s_column_values)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_d \n"
             "BEFORE DELETE ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
             "BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (\n"
                    "'INSERT INTO ") % s_table % QStringLiteral("(rowid,") % s_column_names % QStringLiteral(") "
                        "VALUES('||OLD.rowid||'") % s_column_values % QStringLiteral(");', \n"
                    RESQUN_FUN_GETID "()\n"
                ");\n"
             "END;\n");
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
 *     BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (
 *            'UPDATE Test SET data WHERE rowid='||OLD.rowid||';',
 *            resqun_getid()
 *         );
 *     END;
 * @endcode
 */
QString ReSqliteUnUtil::sqlUpdateTriggerPerColumn (
        const QString &s_table, const QString &s_column)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_u_") % s_column % QStringLiteral(" \n"
             "AFTER UPDATE OF ") % s_column % QStringLiteral(" "
                 "ON ") % s_table % QStringLiteral(" "
                 "WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
             "BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (\n"
                     "'UPDATE ") % s_table % QStringLiteral(" SET ") % s_column % QStringLiteral(" "
                         "WHERE rowid='||OLD.rowid||';',\n"
                     RESQUN_FUN_GETID "()\n"
                 ");\n"
             "END;\n");
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
 *     BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (
 *            'UPDATE Test SET data='||quote(OLD.data)||',data1='||quote(OLD.data1)||' WHERE rowid='||OLD.rowid||';',
 *            resqun_getid()
 *         );
 *     END;
 * @endcode
 *
 */
QString ReSqliteUnUtil::sqlUpdateTriggerPerTable (
        const QString &s_table, const QString & s_column_list)
{
    return QStringLiteral("CREATE TEMP TRIGGER " RESQUN_PREFIX) % s_table % QStringLiteral("_u \n"
         "AFTER UPDATE ON ") % s_table % QStringLiteral(" WHEN (SELECT " RESQUN_FUN_ACTIVE "())=1 \n"
         "BEGIN INSERT INTO resqun_sqlite_undo(sql,idxid) VALUES (\n"
                  "'UPDATE ") % s_table % QStringLiteral(" SET ") % s_column_list % QStringLiteral(" "
                      "WHERE rowid='||OLD.rowid||';',\n"
                  RESQUN_FUN_GETID "()\n"
             ");\n"
                                   "END;\n");
}
/* ========================================================================= */
#include <QWidget>
#include <QVBoxLayout>
#include <QTableView>
#include <QSqlRelationalTableModel>

class DebugViewManager : public QObject {
    Q_OBJECT
public:
    QWidget * wdg;
    QTableView * tv1;
    QTableView * tv2;
    QSqlTableModel *model1;
    QSqlTableModel *model2;

    DebugViewManager(QSqlDatabase database, QWidget * parent) :
        QObject ()
    {
        wdg = new QWidget(parent);
        wdg->setAttribute (Qt::WA_DeleteOnClose);
        this->setParent (wdg);
        QVBoxLayout * main_lay = new QVBoxLayout (wdg);

        tv1 = new QTableView (wdg);
        model1 = new QSqlTableModel (
                    tv1, database);
        model1->setTable (RESQUN_TBL_IDX);
        model1->setEditStrategy (QSqlTableModel::OnFieldChange);
        model1->select ();
        model1->setHeaderData (0, Qt::Horizontal, tr ("ID"));
        model1->setHeaderData (1, Qt::Horizontal, tr ("Name"));
        model1->setHeaderData (1, Qt::Horizontal, tr ("Status"));
        tv1->setModel (model1);
        main_lay->addWidget (tv1);

        tv2 = new QTableView (wdg);
        model2 = new QSqlTableModel (
                    tv2, database);
        model2->setTable (RESQUN_TBL_TEMP);
        model2->setEditStrategy (QSqlTableModel::OnFieldChange);
        model2->select ();
        model2->setHeaderData (0, Qt::Horizontal, tr ("ID"));
        model2->setHeaderData (1, Qt::Horizontal, tr ("SQ"));
        model2->setHeaderData (2, Qt::Horizontal, tr ("Idx"));
//        model2->setRelation (
//                    2, QSqlRelation(RESQUN_TBL_IDX, "id", "name"));
        tv2->setModel (model2);
        main_lay->addWidget (tv2);

        wdg->setLayout (main_lay);

        startTimer(500);
    }

    virtual void timerEvent (QTimerEvent *event) {
        model1->select ();
        model2->select ();
    }

};

/* ------------------------------------------------------------------------- */
QWidget *ReSqliteUnUtil::createDebugView (
        QSqlDatabase database, QWidget * parent)
{
    DebugViewManager * mmm = new DebugViewManager (database, parent);
    return mmm->wdg;
}
/* ========================================================================= */

#include "resqliteun-util.moc"

/*  CLASS    =============================================================== */
//
//
//
//
/* ------------------------------------------------------------------------- */
/* ========================================================================= */

