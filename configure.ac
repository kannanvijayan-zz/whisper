
AC_INIT([whisper], [0.0.1], [kannan@vijayan.ca])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

echo "==== Testing for C++ compiler"
AC_PROG_CXX
AC_LANG([C++])

AC_ARG_ENABLE(debug, [  --enable-debug          Enable debugging.])
AC_ARG_ENABLE(spew, [  --enable-spew           Enable spew.])

AC_CHECK_HEADER([iostream],
                [AC_DEFINE([HAVE_IOSTREAM], [1],
                    [Define to 1 if <iostream> is available])],
                [AC_MSG_ERROR([<iostream> not found])])

AC_CHECK_HEADER([sys/mman.h],
                [AC_DEFINE([HAVE_SYS_MMAN_H], [1],
                    [Define to 1 if <sys/mman.h> is available])],
                [AC_MSG_ERROR([<sys/mman.h> not found])])

AC_CHECK_LIB([unistring], [uc_is_general_category],
                [AC_DEFINE([HAVE_LIBUNISTRING], [1],
                    [Define to 1 if libunistring is available])
                 LIBS="$LIBS -lunistring"],
                [AC_MSG_ERROR([libunistring not found])])

AC_CHECK_LIB([pthread], [pthread_create],
                [AC_DEFINE([HAVE_LIBPTHREAD], [1],
                    [Define to 1 if libpthread is available])
                 LIBS="$LIBS -lpthread"],
                [AC_MSG_ERROR([libpthread not found])])

AC_FUNC_MMAP

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
if test "$enable_debug" = "yes"; then
    AC_DEFINE([ENABLE_DEBUG], [],
                [Define to enable debugging.])
    AC_DEFINE([ENABLE_SPEW], [],
                [Define to enable spew.])
    AM_CONDITIONAL(DEBUG, test x"$enable_debug" = x"yes")
    AM_CONDITIONAL(SPEW, test x"$enable_spew" = x"yes")
fi

if test "$enable_spew" = "yes"; then
    AC_DEFINE([ENABLE_SPEW], [],
                [Define to enable spew.])
    AM_CONDITIONAL(SPEW, test x"$enable_spew" = x"yes")
fi

AC_OUTPUT
