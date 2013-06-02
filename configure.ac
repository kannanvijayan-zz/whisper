
AC_INIT([whisper], [0.0.1], [kannan@vijayan.ca])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

echo "==== Testing for C++ compiler"
AC_PROG_CXX
AC_LANG([C++])

AC_ARG_ENABLE(debug, [  --enable-debug          Enable debugging.])

AC_CHECK_HEADER([cstdint],
                [AC_DEFINE([HAVE_CSTDINT], [1],
                    [Define to 1 if <cstdint> is available])],
                [AC_MSG_ERROR([<cstdint> not found])])

AC_CHECK_HEADER([iostream],
                [AC_DEFINE([HAVE_CSTDINT], [1],
                    [Define to 1 if <iostream> is available])],
                [AC_MSG_ERROR([<iostream> not found])])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/whisper/Makefile
])

# Set architecture word bit-size and byte-size
case "$target_cpu" in
    i*86)
        AC_DEFINE([ARCH_BITS_32], [],
                    [Define if 32-bit arch.])
        ;;
    x86_64)
        AC_DEFINE([ARCH_BITS_64], [],
                    [Define if 64-bit arch.])
        ;;
esac

# Set ENABLE_DEBUG define.
if test "$enable_debug" == "yes"; then
    AC_DEFINE([ENABLE_DEBUG], [],
                [Define to enable debugging.])
fi

AC_OUTPUT
