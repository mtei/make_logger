make_logger
===========

Tiny logger for recursive make for 'make' command

Build and Install
-----------------

    $ make make_logger
    $ cp make_logger ~/bin
      or
    $ sudo install make_logger /usr/local/bin

Usage
-----

### simple example

    $ export MAKE_LOGGER_LOG=`pwd`/logfile.txtq
    $ make_logger <make command arguments>...
    $ cat logfile.txt

### realtime watching example

    $ export MAKE_LOGGER_LOG=`pwd`/logfile.txtq
    $ touch $MAKE_LOGGER_LOG
    $ xterm -e tail -f $MAKE_LOGGER_LOG &
    $ make_logger <make command arguments>...

Copyright
---------

Copyright (c) 2012 ISHII Takeshi. See license.txt for details.
