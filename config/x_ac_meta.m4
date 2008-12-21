##*****************************************************************************
## $Id: x_ac_meta.m4 525 2006-07-13 19:30:13Z dun $
##*****************************************************************************
#  AUTHOR:
#    Chris Dunlap <cdunlap@llnl.gov>
#
#  SYNOPSIS:
#    X_AC_META
#
#  DESCRIPTION:
#    Read metadata from the META file.
##*****************************************************************************

AC_DEFUN([X_AC_META], [
  AC_MSG_CHECKING([metadata])

  META="$srcdir/META"
  _x_ac_meta_got_file=no
  if test -f "$META"; then
    _x_ac_meta_got_file=yes

    META_NAME=_X_AC_META_GETVAL([(?:NAME|PROJECT|PACKAGE)]);
    if test -n "$META_NAME"; then
      AC_DEFINE_UNQUOTED([META_NAME], ["$META_NAME"],
        [Define the project name.]
      )
      AC_SUBST([META_NAME])
    fi

    META_VERSION=_X_AC_META_GETVAL([VERSION]);
    if test -n "$META_VERSION"; then
      AC_DEFINE_UNQUOTED([META_VERSION], ["$META_VERSION"],
        [Define the project version.]
      )
      AC_SUBST([META_VERSION])
    fi

    META_RELEASE=_X_AC_META_GETVAL([RELEASE]);
    if test -n "$META_RELEASE"; then
      AC_DEFINE_UNQUOTED([META_RELEASE], ["$META_RELEASE"],
        [Define the project release.]
      )
      AC_SUBST([META_RELEASE])
    fi

    if test -n "$META_NAME" -a -n "$META_VERSION"; then
        META_ALIAS="$META_NAME-$META_VERSION"
        test -n "$META_RELEASE" -a "$META_RELEASE" != "1" \
          && META_ALIAS="$META_ALIAS-$META_RELEASE"
        AC_DEFINE_UNQUOTED([META_ALIAS], ["$META_ALIAS"],
          [Define the project alias string (name-ver or name-ver-rel).]
        )
        AC_SUBST([META_ALIAS])
    fi

    META_DATE=_X_AC_META_GETVAL([DATE]);
    if test -n "$META_DATE"; then
      AC_DEFINE_UNQUOTED([META_DATE], ["$META_DATE"],
        [Define the project release date.]
      )
      AC_SUBST([META_DATE])
    fi

    META_AUTHOR=_X_AC_META_GETVAL([AUTHOR]);
    if test -n "$META_AUTHOR"; then
      AC_DEFINE_UNQUOTED([META_AUTHOR], ["$META_AUTHOR"],
        [Define the project author.]
      )
      AC_SUBST([META_AUTHOR])
    fi

    m4_pattern_allow([^LT_(CURRENT|REVISION|AGE)$])
    META_LT_CURRENT=_X_AC_META_GETVAL([LT_CURRENT]);
    META_LT_REVISION=_X_AC_META_GETVAL([LT_REVISION]);
    META_LT_AGE=_X_AC_META_GETVAL([LT_AGE]);
    if test -n "$META_LT_CURRENT" \
         -o -n "$META_LT_REVISION" \
         -o -n "$META_LT_AGE"; then
      test -n "$META_LT_CURRENT" || META_LT_CURRENT="0"
      test -n "$META_LT_REVISION" || META_LT_REVISION="0"
      test -n "$META_LT_AGE" || META_LT_AGE="0"
      AC_DEFINE_UNQUOTED([META_LT_CURRENT], ["$META_LT_CURRENT"],
        [Define the libtool library 'current' version information.]
      )
      AC_DEFINE_UNQUOTED([META_LT_REVISION], ["$META_LT_REVISION"],
        [Define the libtool library 'revision' version information.]
      )
      AC_DEFINE_UNQUOTED([META_LT_AGE], ["$META_LT_AGE"],
        [Define the libtool library 'age' version information.]
      )
      AC_SUBST([META_LT_CURRENT])
      AC_SUBST([META_LT_REVISION])
      AC_SUBST([META_LT_AGE])
    fi
  fi

  AC_MSG_RESULT([$_x_ac_meta_got_file])
  ]
)

AC_DEFUN([_X_AC_META_GETVAL],
  [`perl -n\
    -e "BEGIN { \\$key=shift @ARGV; }"\
    -e "next unless s/^\s*\\$key@<:@:=@:>@//i;"\
    -e "s/^((?:@<:@^'\"#@:>@*(?:(@<:@'\"@:>@)@<:@^\2@:>@*\2)*)*)#.*/\\@S|@1/;"\
    -e "s/^\s+//;"\
    -e "s/\s+$//;"\
    -e "s/^(@<:@'\"@:>@)(.*)\1/\\@S|@2/;"\
    -e "\\$val=\\$_;"\
    -e "END { print \\$val if defined \\$val; }"\
    '$1' $META`]dnl
)
