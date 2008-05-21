##
# Prologue
##
AC_INIT([scrub])
AC_META
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_SYSTEM

##
# Automake support
##
AM_INIT_AUTOMAKE($PROJECT, $VERSION)
AM_CONFIG_HEADER([config/config.h])
AM_MAINTAINER_MODE

##
# Checks for programs
##
AC_PROG_CC

##
# Checks for header files.
##
AC_HEADER_STDC
AC_CHECK_HEADERS( \
  getopt.h \
  sys/mode.h \
)

##
# Checks for typedefs, structures, and compiler characteristics
## 
AC_C_BIGENDIAN
AC_C_CONST

##
# Checks for library functions
##
AC_CHECK_FUNCS( \
  getopt_long \
)

##
# Arrange for large file support
## 
AC_SYS_LARGEFILE

##
# Epilogue
##
AC_CONFIG_FILES( \
  Makefile \
  src/Makefile \
  test/Makefile \
)
AC_OUTPUT
