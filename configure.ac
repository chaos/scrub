##
# Prologue
##
AC_INIT(m4_esyscmd([awk '/Name:/ {printf "%s",$2; exit}' META]),
        m4_esyscmd([awk '/Version:/ {printf "%s",$2; exit}' META]))

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_SYSTEM
X_AC_META
X_AC_EXPAND_INSTALL_DIRS

##
# Automake support
##
AM_INIT_AUTOMAKE($META_NAME, $META_VERSION)
AM_CONFIG_HEADER([config/config.h])
AM_MAINTAINER_MODE

##
# Checks for programs
##
AC_PROG_CC

##
# Checks for header files.
##
AC_USE_SYSTEM_EXTENSIONS
AC_HEADER_STDC
AC_CHECK_HEADERS( \
  getopt.h \
  stdbool.h \
  pthread.h \
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
  posix_memalign \
  memalign \
  posix_fadvise \
  random_r \
)
X_AC_CHECK_PTHREADS

##
# Arrange for large file support
## 
AC_SYS_LARGEFILE

##
# Epilogue
##
AC_CONFIG_FILES( \
  Makefile \
  scrub.spec \
  src/Makefile \
  man/Makefile \
  man/scrub.1 \
  test/Makefile \
)
AC_OUTPUT
