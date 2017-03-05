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

#include <resqliteun/resqliteun-manager.h>
#include <resqliteun/resqliteun-util.h>

#include <QString>
#include <QList>

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
/*  CLASS    --------------------------------------------------------------- */

//! Adds undo-redo capabilities to a sqlite database.
class RESQLITEUN_EXPORT ReSqliteUn :
        public ReSqliteUnManager, public ReSqliteUnUtil {
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
    bool is_active_; /**< is the instance active  or not? */
    bool in_undo_; /**< are we performing an undo or a redo (valid when is_active_) */

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
    virtual ~ReSqliteUn ();


    //! Creates a restore point.
    ReSqliteUn::SqLiteResult
    begin (
            const QString &s_name,
            qint64 * entry_id=NULL);

    //! Closes a restore point.
    ReSqliteUn::SqLiteResult
    end ();

    //! Creates the triggers for mentioned table.
    ReSqliteUn::SqLiteResult
    attachToTable (
            const QString &table,
            UpdateBehaviour update_kind);

    //! Do an Undo or Redo.
    ReSqliteUn::SqLiteResult
    performUndoRedo (
            bool for_undo,
            QString &s_error);

    //! Do multiple Undo or Redo.
    ReSqliteUn::SqLiteResult
    performUndoRedo (
            int steps,
            bool for_undo,
            QString &s_error);

    //! Get the number of steps required to reach a certain id.
    ReSqliteUn::SqLiteResult
    stepsToGoal (
            bool for_undo,
            qint64 goal_id,
            int &steps);

    //! Get the number of entries in the temporary table by kind.
    SqLiteResult
    count (
            qint64 & undo_entries,
            qint64 & redo_entries) const;

    //! Get the id of the undo or redo entry that should be used right now.
    qint64
    getActiveId (
            UndoRedoType ty = CurrentUndoRedo) const;

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
