/**
 * @file resqliteun-names.h
 * @brief The names used by the library.
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_RESQLITEUN_NAMES_H_INCLUDE
#define GUARD_RESQLITEUN_NAMES_H_INCLUDE


/**
 * @defgroup RESQUN_STRINGS text strings used by the module
 *
 * For convenience the strings used by the plugin that might be useful
 * to the user are defined here; they are also available as static
 * members in the ReSqliteUn class.
 * @{
 */

#ifndef RESQUN_PREFIX
//! The prefix used by all methods inside sqlite environment.
#define RESQUN_PREFIX       "resqun_"
#endif // RESQUN_PREFIX

#ifndef RESQUN_FUN_TABLE
//! Name of the function used for adding a table to the list monitored by this module.
#define RESQUN_FUN_TABLE    RESQUN_PREFIX "table"
#endif // RESQUN_FUN_TABLE

#ifndef RESQUN_FUN_ACTIVE
//! Name of the function used for
#define RESQUN_FUN_ACTIVE   RESQUN_PREFIX "active"
#endif // RESQUN_FUN_ACTIVE

#ifndef RESQUN_FUN_BEGIN
//! Name of the function used for
#define RESQUN_FUN_BEGIN    RESQUN_PREFIX "begin"
#endif // RESQUN_FUN_BEGIN

#ifndef RESQUN_FUN_END
//! Name of the function used for .
#define RESQUN_FUN_END      RESQUN_PREFIX "end"
#endif // RESQUN_FUN_END

#ifndef RESQUN_FUN_UNDO
//! Name of the function used for performing an undo step.
#define RESQUN_FUN_UNDO     RESQUN_PREFIX "undo"
#endif // RESQUN_FUN_UNDO

#ifndef RESQUN_FUN_REDO
//! Name of the function used for performing an redo step.
#define RESQUN_FUN_REDO     RESQUN_PREFIX "redo"
#endif // RESQUN_FUN_REDO

#ifndef RESQUN_FUN_GETID
//! Name of the function used for performing an redo step.
#define RESQUN_FUN_GETID    RESQUN_PREFIX "getid"
#endif // RESQUN_FUN_GETID

#ifndef RESQUN_TBL_TEMP
//! The table to be used for storing undo-redo stack.
#define RESQUN_TBL_TEMP     RESQUN_PREFIX "sqlite_undo"
#endif // RESQUN_TBL_TEMP

#ifndef RESQUN_TBL_IDX
//! The table to be used for storing undo-redo indices.
#define RESQUN_TBL_IDX      RESQUN_PREFIX "sqlite_itbl"
#endif // RESQUN_TBL_IDX

#ifndef RESQUN_INDEX_DATA
//! The table to be used for storing undo-redo indices.
#define RESQUN_INDEX_DATA   RESQUN_PREFIX "sqlite_index"
#endif // RESQUN_INDEX_DATA

#ifndef RESQUN_SVP_BEGIN
//! Name of the savepoint used in `begin` command.
#define RESQUN_SVP_BEGIN    RESQUN_PREFIX "begin_svp"
#endif // RESQUN_SVP_BEGIN

#ifndef RESQUN_SVP_UNDO
//! Name of the savepoint used in `undo` command.
#define RESQUN_SVP_UNDO     RESQUN_PREFIX "und_svp"
#endif // RESQUN_SVP_UNDO

#ifndef RESQUN_MARK_UNDO
//! Marker used in temporary table to indicate an UNDO entry.
#define RESQUN_MARK_UNDO    0
#endif // RESQUN_MARK_UNDO

#ifndef RESQUN_MARK_REDO
//! Marker used in temporary table to indicate an REDO entry.
#define RESQUN_MARK_REDO    1
#endif // RESQUN_MARK_REDO


/** @} */



#endif // GUARD_RESQLITEUN_NAMES_H_INCLUDE
