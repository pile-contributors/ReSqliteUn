ReSqliteUn
==========

ReSqliteUn pile provides undo-redo for sqlite in the form of a plugin.

The code was only tested against version 3.16.2 of the sqlite source
distribution that may be found in the helper repository.


> This plugin was inspired by sqlite-undo project available at
> https://sourceforge.net/projects/sqlite-undo. However,
> as that code is under [GPL v3 license][7], a new implementation was
> required by the project that I currently work on. The names of the
> functions are similar to those used in that code, with a unique
> prefix to distinguish between them.
> (Nicu Tofan, February 26, 2017).

[1]: https://sourceforge.net/projects/sqlite-undo
[2]: http://sqlite.mobigroup.ru/artifact/265e408b4352d66cfc79a9990cb2c22fb390d3b6
[3]: http://sqlite.mobigroup.ru/artifact/2250bbbc83f80eff73ce003ab7a30293c688ae9b
[4]: https://www.sqlite.org/undoredo.html
[5]: https://www.sqlite.org/loadext.html
[6]: https://www.sqlite.org/c3ref/create_function.html
[7]: https://sourceforge.net/p/sqlite-undo/code/HEAD/tree/LICENSE
