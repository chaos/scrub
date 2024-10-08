##
# Prologue
##
AC_INIT([scrub],
        m4_esyscmd([git describe --tags --always | awk '/.*/ {sub(/^v/, ""); printf "%s",$1; exit}']))

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_SYSTEM
AC_CONFIG_MACRO_DIR([config])
X_AC_EXPAND_INSTALL_DIRS

##
# Automake support
##
AM_INIT_AUTOMAKE([subdir-objects foreign])
AM_SILENT_RULES([yes])
AM_CONFIG_HEADER([config/config.h])
AM_MAINTAINER_MODE


##
# Conditionally build libscrub
##
AC_ARG_ENABLE(libscrub,
[  --enable-libscrub       Build libscrub],
[case "${enableval}" in
  yes) libscrub=true ;;
  no) libscrub=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-libscrub) ;;
esac],[libscrub=false])
AM_CONDITIONAL(LIBSCRUB, test x$libscrub = xtrue)

##
# Checks for programs
##
AC_PROG_CC
AM_PROG_CC_C_O

##
# Checks for header files.
##
AC_USE_SYSTEM_EXTENSIONS
AC_HEADER_STDC
AC_CHECK_HEADERS( \
  getopt.h \
  stdbool.h \
  stdint.h \
  pthread.h \
  linux/fs.h \
  sys/devinfo.h \
  sys/disk.h \
  sys/dkio.h \
  sys/ioctl.h \
  sys/scsi.h \
  sys/mman.h \
)

AC_PROG_LIBTOOL
AC_PKGCONFIG

##
# Checks for typedefs, structures, and compiler characteristics
## 
AC_C_BIGENDIAN
AC_C_CONST

##
# Checks for libraries
##
AC_CHECK_LIB(prop, prop_dictionary_recv_ioctl, LIBPROP=-lprop)
AC_SUBST(LIBPROP)

##
# Checks for library functions
##
AC_CHECK_FUNCS( \
  getopt_long \
  posix_memalign \
  memalign \
  posix_fadvise \
  rand_r \
  random_r \
)
X_AC_CHECK_PTHREADS


# Sanity check; we cannot have both --with-libgcrypt AND --with-openssl
# together.
AS_IF([test "x$with_openssl" = "xyes"], [
  AS_IF([test "x$with_libgcrypt" = "xyes"],
    [AC_MSG_ERROR([You can use either --with-openssl or --with-libgcrypt, not both at once])]
  )
])

##
# OpenSSL libcrypto library
##
have_openssl=no
AC_ARG_WITH(openssl, AS_HELP_STRING([--with-openssl], [build with OpenSSL libcrypto]))

if test "x$with_openssl" = "xyes"; then
  AC_SEARCH_LIBS([RAND_bytes], [crypto],
    [AC_DEFINE([HAVE_OPENSSL], [1], [OpenSSL libcrypto available])
      have_openssl=yes
    ], [AC_MSG_ERROR([OpenSSL libcrypto required])]
  )
fi

##
# gcrypt library
##
have_libgcrypt=no
AC_ARG_WITH(libgcrypt, AS_HELP_STRING([--without-libgcrypt], [build without libgcrypt;
                                         fallback to custom AES implementation]))

# Technically there is no need for testing this again, as we already
# error'ed out early if both options were enabled at once.
AS_IF([test "x$with_openssl" != "xyes"], [
  AS_IF([test "x$with_libgcrypt" != "xno"], [
    AM_PATH_LIBGCRYPT([1.5.0],
      [AC_DEFINE([HAVE_LIBGCRYPT], [1], [libgcrypt API available])
        gcrypt_CFLAGS="$LIBGCRYPT_CFLAGS"
        gcrypt_LIBS="$LIBGCRYPT_LIBS"
        have_libgcrypt=yes
      ]
    )]
  )]
)
AM_CONDITIONAL([LIBGCRYPT], [test "$have_libgcrypt" = "yes"])
AC_SUBST([gcrypt_CFLAGS])
AC_SUBST([gcrypt_LIBS])

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
  libscrub/Makefile \
  libscrub/libscrub.pc \
)
AC_OUTPUT
