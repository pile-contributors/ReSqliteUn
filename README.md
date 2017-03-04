ReSqliteUn
==========

ReSqliteUn pile provides undo-redo for sqlite in the form of a plugin.

The code was only tested against version 3.16.2 of the sqlite source
distribution that may be found in the helper repository.

> This plugin was inspired by sqlite-undo project by Simon Naunton available at
> https://sourceforge.net/projects/sqlite-undo. However,
> as that code is under [GPL v3 license][7], a new implementation was
> required by the project that I currently work on. The names of the sql
> functions are similar to those used in that code, with a unique
> prefix to distinguish between them.
> (Nicu Tofan, February 26, 2017).

Usage
-----

The user of the project has two options:
- use this as a regular plugin paacked as a dynamic library in the way
described in [Run-Time Loadable Extensions][5];
- include the source code for the project and call `sqlite3_resqliteun_init()`
for each database or `ReSqliteUn::autoregister ()` for all databases.

Next, the user may issue commands using the familiar `SELECT ...` mechanism.
Following functions are available:
- resqun_table: add a table to the list of those that are monitores; once
attached to a table the monitoring mechanism cannot be detached;
- resqun_begin: start a new sequence that should be bundled together
in a single undo step;
- resqun_end: finish an undo step; statements issues against the monitored
table between an `end` and a `begin` are not recorded;
- resqun_active: tells if `begin` was called and `end` was not yet called
(statements are being tracked if result is 1; are not if result is 0);
- resqun_undo:  take last step in the undo stack and un-do its effects;
a new `redo` entry will also be created;
- resqun_redo: take last step in the redo stack and apply it;
a new `undo` entry will also be created;

The library also has a binay interface by using
the methods of the ReSqliteUn class; with one ReSqliteUn class attached to
each database. Last instance that was created is available through
ReSqliteUn::instance() and to find the instance for a particular
`sqlite3 *` pointer use ReSqliteUn::instanceForDatabase().




Implementation
--------------

In order for the



[1]: https://sourceforge.net/projects/sqlite-undo
[2]: http://sqlite.mobigroup.ru/artifact/265e408b4352d66cfc79a9990cb2c22fb390d3b6
[3]: http://sqlite.mobigroup.ru/artifact/2250bbbc83f80eff73ce003ab7a30293c688ae9b
[4]: https://www.sqlite.org/undoredo.html
[5]: https://www.sqlite.org/loadext.html
[6]: https://www.sqlite.org/c3ref/create_function.html
[7]: https://sourceforge.net/p/sqlite-undo/code/HEAD/tree/LICENSE

