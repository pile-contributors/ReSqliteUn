/* ========================================================================= */
/* ------------------------------------------------------------------------- */
/**
 * @file resqliteun-manager.h
 * @brief Declarations for ReSqliteUnManager class
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_RESQLITEUN_MANAGER_H_INCLUDE
#define GUARD_RESQLITEUN_MANAGER_H_INCLUDE

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
#include <QCoreApplication>

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

//! The management routines.
class RESQLITEUN_EXPORT ReSqliteUnManager {
    //
    //
    //
    //
    /*  DEFINITIONS    ----------------------------------------------------- */
    Q_DECLARE_TR_FUNCTIONS(ReSqliteUnManager)

    typedef void(*xEntryPoint)(void);

    /*  DEFINITIONS    ===================================================== */
    //
    //
    //
    //
    /*  DATA    ------------------------------------------------------------ */

public:

    static QList<ReSqliteUn *> instances_; /**< the list of instances */

    /*  DATA    ============================================================ */
    //
    //
    //
    //
    /*  FUNCTIONS    ------------------------------------------------------- */

public:

    //! Default constructor.
    ReSqliteUnManager () {}

    //! Destructor.
    virtual ~ReSqliteUnManager () {}

    //! The last instance.
    static ReSqliteUn *
    instance (
            int interface_version=RESQLITEUN_VERSION);

    //! The number of instances.
    static int
    instanceCount () {
        return instances_.count ();
    }

    //! The instance at a particular index.
    static ReSqliteUn *
    instanceForIndex (
            int i,
            int interface_version=RESQLITEUN_VERSION);

    //! The one and only instance.
    static ReSqliteUn *
    instanceForDatabase (
            void * sqlite_database,
            int interface_version=RESQLITEUN_VERSION);

    //! Creates an instance of the clsaa for the given database.
    static ReSqliteUn *
    create (
            void * db,
            QString & s_error);

    //! The entry point for the sqlite3.
    static xEntryPoint
    getEntryPoint ();

    //! Autoregister this extension with each new database
    //! (when not using the plugin).
    static bool
    autoregister ();

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

#endif // GUARD_RESQLITEUN_MANAGER_H_INCLUDE

/* ------------------------------------------------------------------------- */
/* ========================================================================= */
