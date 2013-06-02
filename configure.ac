
AC_INIT([whisper], [0.0.1], [kannan@vijayan.ca])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

echo "==== Testing for C++ compiler"
AC_PROG_CXX
AC_LANG([C++])

AC_ARG_ENABLE(debug, [  --enable-debug          Enable debugging.])

AC_CHECK_HEADER([cstdint],
                [AC_DEFINE([HAVE_CSTDINT], [1],
                    [Define to 1 if you have <cstdint>])],
                [AC_MSG_ERROR([<cstdint> not found])])

AC_CONFIG_HEADERS([
    src/config.h
])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])

AC_OUTPUT
