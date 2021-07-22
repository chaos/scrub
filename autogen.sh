#!/bin/sh

echo "Running aclocal ... "
aclocal -I config

if glibtoolize --version 1>/dev/null 2>&1; then
  LIBTOOLIZE=glibtoolize
fi
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
echo "Running libtoolize ... "
"$LIBTOOLIZE" --force --automake --copy

echo "Running autoheader ... "
autoheader
echo "Running automake ... "
automake --copy --add-missing
echo "Running autoconf ... "
autoconf
echo "Cleaning up ..."
mv aclocal.m4 config/
rm -rf autom4te.cache
echo "Now run ./configure to configure scrub for your environment."
