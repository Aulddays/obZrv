AC_INIT([obZrv], [0.1], [], [obZrv])
AC_CONFIG_SRCDIR([obZrv/main.cpp])

# Auto-detect MinGW cross-compiler on Linux if --host not specified
AS_IF([test "x$host_alias" = "x"],
  [AC_CHECK_PROGS([MINGW_CXX], [i686-w64-mingw32-g++], [no])
   AS_IF([test "x$MINGW_CXX" != "xno"],
     [host_alias=$(echo "$MINGW_CXX" | sed 's/-g++$//')
      CXX=$MINGW_CXX
      CC=$(echo "$MINGW_CXX" | sed 's/-g++$/-gcc/')
      AR=${host_alias}-ar
      RANLIB=${host_alias}-ranlib
      cross_compiling=yes
      AC_MSG_NOTICE([Auto-detected MinGW cross-compiler: $host_alias])],
     [AC_MSG_NOTICE([No MinGW cross-compiler found, using native])])])

AM_INIT_AUTOMAKE([foreign -Wall])
AC_CANONICAL_HOST
AC_PROG_CXX
AM_PROG_AR
AC_PROG_RANLIB

# Find windres and strip with cross-compile prefix if needed
AS_IF([test "x$host_alias" != "x"],
  [AC_CHECK_PROGS([WINDRES], [${host_alias}-windres windres], [no])
   AC_CHECK_PROGS([STRIP],   [${host_alias}-strip   strip],   [strip])],
  [AC_CHECK_TOOL([WINDRES], [windres], [:])
   AC_CHECK_TOOL([STRIP],   [strip],   [strip])])

# WinXP compatibility + Unicode
CPPFLAGS="$CPPFLAGS -DWINVER=0x0501 -D_WIN32_WINNT=0x0501"
CPPFLAGS="$CPPFLAGS -DUNICODE -D_UNICODE"

# On Windows (MinGW) asio requires Winsock2 and Mswsock libraries.
WINDOWS_SOCKET_LIBS=""
case "$host" in
    *-*-mingw*|*-*-cygwin*)
        WINDOWS_SOCKET_LIBS="-lws2_32 -lmswsock"
        ;;
esac
AC_SUBST([WINDOWS_SOCKET_LIBS])

AC_CONFIG_FILES([Makefile obZrv/Makefile unifs/Makefile])
AC_OUTPUT
