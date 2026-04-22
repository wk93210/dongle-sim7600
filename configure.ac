dnl chan_dongle - SIM7600/SIMCOM Channel Driver for Asterisk
AC_PREREQ([2.69])
AC_INIT([chan_dongle], [1.1], [https://github.com/wk93210/sip/issues])
AC_CONFIG_SRCDIR([chan_dongle.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

# Platform detection
AC_CANONICAL_HOST
case "$host_os" in
  darwin*)
    AC_DEFINE([IS_DARWIN], [1], [Building on macOS])
    ;;
  cygwin*|mingw*)
    AC_DEFINE([IS_CYGWIN], [1], [Building on Windows])
    ;;
  linux*)
    AC_DEFINE([IS_LINUX], [1], [Building on Linux])
    ;;
  freebsd*|openbsd*|netbsd*)
    AC_DEFINE([IS_BSD], [1], [Building on BSD])
    ;;
esac

# Checks for programs
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
LT_INIT([disable-static dlopen])
AM_PROG_CC_C_O

# Check for C11 support
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T

# Checks for header files
AC_CHECK_HEADERS([fcntl.h inttypes.h stddef.h stdint.h stdlib.h string.h strings.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for library functions
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memchr memmem memmove memset realpath strcasecmp strchr strncasecmp strtol])

# pthread
AC_SEARCH_LIBS([pthread_create], [pthread], [], [AC_MSG_ERROR([pthread library required])])

# sqlite3
PKG_CHECK_MODULES([SQLITE3], [sqlite3], [], [
  AC_SEARCH_LIBS([sqlite3_open], [sqlite3], [], [AC_MSG_ERROR([sqlite3 library required])])
])

# iconv
AC_ARG_WITH([iconv],
  AS_HELP_STRING([--with-iconv=DIR], [iconv installation directory]),
  [ICONV_PREFIX="$withval"],
  [ICONV_PREFIX=""])

if test -n "$ICONV_PREFIX"; then
  ICONV_CFLAGS="-I${ICONV_PREFIX}/include"
  ICONV_LIBS="-L${ICONV_PREFIX}/lib -liconv"
else
  AC_SEARCH_LIBS([iconv], [iconv], [ICONV_LIBS=""], [ICONV_LIBS=""])
  ICONV_CFLAGS=""
fi

# Check for iconv_t type and const usage
AC_MSG_CHECKING([for iconv_t])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <iconv.h>]], [[iconv_t cd;]])],
  [
    AC_MSG_RESULT([yes])
    AC_DEFINE([ICONV_T], [iconv_t], [Define to iconv_t if available])
  ],
  [
    AC_MSG_RESULT([no])
    AC_DEFINE([ICONV_T], [int], [Define to int if iconv_t not available])
  ])

AC_MSG_CHECKING([whether iconv input is const])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <iconv.h>
#include <stddef.h>
]], [[iconv_t cd; const char *in; size_t inb; char *out; size_t outb; iconv(cd, &in, &inb, &out, &outb);]])],
  [
    AC_MSG_RESULT([yes])
    AC_DEFINE([ICONV_CONST], [const], [Define to const if iconv input is const])
  ],
  [
    AC_MSG_RESULT([no])
    AC_DEFINE([ICONV_CONST], [], [Define to empty if iconv input is not const])
  ])

AC_SUBST([ICONV_CFLAGS])
AC_SUBST([ICONV_LIBS])

# Asterisk detection
AC_MSG_CHECKING([for Asterisk])

AC_ARG_WITH([asterisk],
  AS_HELP_STRING([--with-asterisk=DIR], [Asterisk installation prefix]),
  [ASTERISK_PREFIX="$withval"],
  [ASTERISK_PREFIX=""])

AC_ARG_WITH([asterisk-headers],
  AS_HELP_STRING([--with-asterisk-headers=DIR], [Asterisk headers directory]),
  [ASTERISK_HEADERS="$withval"],
  [ASTERISK_HEADERS=""])

AC_ARG_WITH([asterisk-modules],
  AS_HELP_STRING([--with-asterisk-modules=DIR], [Asterisk modules directory]),
  [ASTERISK_MODULES="$withval"],
  [ASTERISK_MODULES=""])

# Find Asterisk headers
if test -n "$ASTERISK_HEADERS"; then
  ASTERISK_INCLUDE_DIR="$ASTERISK_HEADERS"
else
  # First try pkg-config
  if pkg-config --exists asterisk 2>/dev/null; then
    ASTERISK_INCLUDE_DIR=$(pkg-config --variable=includedir asterisk 2>/dev/null)
    if test -z "$ASTERISK_INCLUDE_DIR"; then
      # pkg-config doesn't give includedir, try cflags
      pkg_cflags=$(pkg-config --cflags asterisk 2>/dev/null)
      for flag in $pkg_cflags; do
        case $flag in
          -I*)
            dir=$(echo "$flag" | sed 's/^-I//')
            if test -f "$dir/asterisk.h"; then
              ASTERISK_INCLUDE_DIR="$dir"
              break
            fi
            ;;
        esac
      done
    fi
  fi

  # If still not found, try common paths
  if test -z "$ASTERISK_INCLUDE_DIR"; then
    ast_search_paths="/usr/include /usr/local/include /opt/asterisk/include"
    if test -n "$ASTERISK_PREFIX"; then
      ast_search_paths="$ASTERISK_PREFIX/include $ast_search_paths"
    fi

    for dir in $ast_search_paths; do
      if test -f "$dir/asterisk.h" || test -f "$dir/asterisk/asterisk.h"; then
        ASTERISK_INCLUDE_DIR="$dir"
        break
      fi
    done
  fi
fi

if test -z "$ASTERISK_INCLUDE_DIR"; then
  AC_MSG_ERROR([Asterisk headers not found. Install asterisk-dev or use --with-asterisk-headers=DIR])
fi

AC_MSG_RESULT([$ASTERISK_INCLUDE_DIR])

# Check for Asterisk version
AC_MSG_CHECKING([for Asterisk version])

asterisk_version_num=0
if test -f "$ASTERISK_INCLUDE_DIR/asterisk/version.h"; then
  # Try to extract version from version.h
  ast_ver=$(grep "ASTERISK_VERSION_NUM" "$ASTERISK_INCLUDE_DIR/asterisk/version.h" 2>/dev/null | head -1 | sed 's/.*\([0-9]\+\).*/\1/')
  if test -n "$ast_ver"; then
    asterisk_version_num=$ast_ver
  fi
fi

# Fallback: try to get version from asterisk binary
if test "$asterisk_version_num" = "0"; then
  if command -v asterisk >/dev/null 2>&1; then
    ast_ver=$(asterisk -V 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
    if test -n "$ast_ver"; then
      # Convert X.Y to version number
      ast_major=$(echo "$ast_ver" | cut -d. -f1)
      ast_minor=$(echo "$ast_ver" | cut -d. -f2)
      asterisk_version_num=$((ast_major * 100000 + ast_minor * 1000))
    fi
  fi
fi

# Check minimum version (we require Asterisk 20)
if test "$asterisk_version_num" -lt "200000"; then
  AC_MSG_RESULT([unknown or old version])
  AC_MSG_WARN([Could not determine Asterisk version or version < 20])
  AC_MSG_WARN([Attempting to build anyway...])
  asterisk_version_num="200190"
else
  AC_MSG_RESULT([$asterisk_version_num])
fi

AC_DEFINE_UNQUOTED([ASTERISK_VERSION_NUM], [$asterisk_version_num], [Asterisk version number])

# Check for Asterisk module directory
if test -z "$ASTERISK_MODULES"; then
  # Try to find modules directory
  ast_mod_paths="/usr/lib/asterisk/modules /usr/local/lib/asterisk/modules /opt/asterisk/lib/asterisk/modules"
  if test -n "$ASTERISK_PREFIX"; then
    ast_mod_paths="$ASTERISK_PREFIX/lib/asterisk/modules $ast_mod_paths"
  fi

  for dir in $ast_mod_paths; do
    if test -d "$dir"; then
      ASTERISK_MODULES="$dir"
      break
    fi
  done

  # Default fallback
  if test -z "$ASTERISK_MODULES"; then
    ASTERISK_MODULES="/usr/lib/asterisk/modules"
  fi
fi

AC_SUBST([ASTERISK_INCLUDE_DIR])
AC_SUBST([ASTERISK_MODULES])

# Check for Asterisk features
ac_save_CPPFLAGS="$CPPFLAGS"
ac_save_cflags="$CFLAGS"
CPPFLAGS="$CPPFLAGS -I$ASTERISK_INCLUDE_DIR"
CFLAGS="$CFLAGS -I$ASTERISK_INCLUDE_DIR"

if test ! -f "$ASTERISK_INCLUDE_DIR/asterisk/frame.h"; then
  AC_MSG_ERROR([asterisk/frame.h not found in $ASTERISK_INCLUDE_DIR/asterisk])
fi

CPPFLAGS="$ac_save_CPPFLAGS"
CFLAGS="$ac_save_cflags"

# AST_CONTROL_SRCCHANGE is available in Asterisk 20+
AC_DEFINE([HAVE_AST_CONTROL_SRCCHANGE], [1], [Define if AST_CONTROL_SRCCHANGE is available])

# Build options
AC_ARG_ENABLE([applications],
  AS_HELP_STRING([--disable-applications], [Disable Asterisk dialplan applications]),
  [enable_applications=$enableval],
  [enable_applications=yes])

AC_ARG_ENABLE([manager],
  AS_HELP_STRING([--disable-manager], [Disable Asterisk Manager Interface extensions]),
  [enable_manager=$enableval],
  [enable_manager=yes])

if test "x$enable_applications" = "xyes"; then
  AC_DEFINE([BUILD_APPLICATIONS], [1], [Build dialplan applications])
fi

if test "x$enable_manager" = "xyes"; then
  AC_DEFINE([BUILD_MANAGER], [1], [Build Manager Interface extensions])
fi

# Module symbol
AC_DEFINE([AST_MODULE], ["chan_dongle"], [Module name])
AC_DEFINE([AST_MODULE_SELF_SYM], [__internal_chan_dongle_self], [Module self symbol])
AC_DEFINE([MODULE_DESCRIPTION], ["SIM7600/SIMCOM Channel Driver"], [Module description])
AC_DEFINE([MODULE_VERSION], ["1.1"], [Module version])
AC_DEFINE([PACKAGE_REVISION], ["git"], [Package revision])
AC_DEFINE([MODULE_URL], ["https://github.com/wk93210/sip"], [Module URL])
AC_DEFINE([MODULE_BUGREPORT], ["https://github.com/wk93210/sip/issues"], [Bug report URL])

# Output files
AC_CONFIG_FILES([
  Makefile
  tests/Makefile
])

AC_OUTPUT

echo "
========================================
$PACKAGE_NAME $VERSION configuration:

  Asterisk version:    $asterisk_version_num
  Asterisk headers:    $ASTERISK_INCLUDE_DIR
  Asterisk modules:    $ASTERISK_MODULES

  Compiler:            $CC
  CFLAGS:              $CFLAGS

  Build applications:  $enable_applications
  Build manager:       $enable_manager

  Install prefix:      $prefix

Run 'make' to build.
Run 'make install' to install to $ASTERISK_MODULES
========================================
"
