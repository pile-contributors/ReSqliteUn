/* ========================================================================= */
/* ------------------------------------------------------------------------- */
/**
 * @file resqliteun-util.h
 * @brief Declarations for ReSqliteUnUtil class
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_RESQLITEUN_UTIL_H_INCLUDE
#define GUARD_RESQLITEUN_UTIL_H_INCLUDE

/* ------------------------------------------------------------------------- */
/* ========================================================================= */
//
//
//
//
/*  INCLUDES    ------------------------------------------------------------ */

#include <resqliteun/resqliteun-config.h>
#include <resqliteun/resqliteun-names.h>

#include <QString>
#include <QList>

/*  INCLUDES    ============================================================ */
//
//
//
//
/*  DEFINITIONS    --------------------------------------------------------- */

class ReSqliteUn;

/*  DEFINITIONS    ========================================================= */
//
//
//
//
/*  CLASS    --------------------------------------------------------------- */

//! The utility routines.
class RESQLITEUN_EXPORT ReSqliteUnUtil {
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

    //! Ways to refer to undo and redo.
    enum UndoRedoType {
        NoUndoRedo = 0,
        UndoType,
        RedoType,
        CurrentUndoRedo,
        BothUndoRedo
    };

    typedef int SqLiteResult;

    /*  DEFINITIONS    ===================================================== */
    //
    //
    //
    //
    /*  DATA    ------------------------------------------------------------ */

    /*  DATA    ============================================================ */
    //
    //
    //
    //
    /*  FUNCTIONS    ------------------------------------------------------- */

public:

    //! Default constructor.
    ReSqliteUnUtil () {}

    //! Destructor.
    virtual ~ReSqliteUnUtil () {}

    //! Get the value of a column as a string.
    static QString
    columnText (
            void * statement,
            int column);

    //! Attach strings.
    static int
    bind (
            void *statement,
            int column,
            const QString &s_value);


    //! Get the content of a value as a string.
    static QString
    value2string (
            void *val);


    //! The sql statements that create triggers for a table.
    static QString
    sqlTriggers (
            void *db,
            const QString &table,
            UpdateBehaviour update_kind);

    //! Compute the sql string for insert trigger.
    static QString
    sqlInsertTrigger (
            const QString &s_table);

    //! Compute the sql string for delete trigger.
    static QString
    sqlDeleteTrigger (
            const QString &s_table,
            const QString & s_column_names,
            const QString & s_column_values);

    //! Compute the sql string for update trigger.
    static QString
    sqlUpdateTriggerPerColumn (
            const QString &s_table,
            const QString &s_colum);

    //! Compute the sql string for update trigger.
    static QString
    sqlUpdateTriggerPerTable (
            const QString &s_table,
            const QString &s_column_list);

    /*  FUNCTIONS    ======================================================= */
    //
    //
    //
    //

}; // class ReSqliteUnUtil

/*  CLASS    =============================================================== */
//
//
//
//

#endif // GUARD_RESQLITEUN_UTIL_H_INCLUDE

/* ------------------------------------------------------------------------- */
/* ========================================================================= */
