#!/bin/sh

# Helps bootstrapping the application when checked out from CVS.
# Requires GNU autoconf, GNU automake and GNU which.
#
# Copyright (C) 2004, by
#
# Carlo Wood, Run on IRC <carlo@alinoe.com>
# RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
# Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
#

# Do sanity checks.
# Directory check.
if [ ! -f autogen.sh ]; then
  echo "Run ./autogen.sh from the directory it exists in."
  exit 1
fi

# Clueless user check.
if test ! -d CVS -a -f configure; then
  echo "You only need to run './autogen.sh' when you checked out this project using CVS."
  echo "Just run ./configure [--help]."
  exit 0
fi

AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOCONF=${AUTOCONF:-autoconf}

($AUTOCONF --version) >/dev/null 2>/dev/null || (echo "You need GNU autoconf to install from CVS (ftp://ftp.gnu.org/gnu/autoconf/)"; exit 1) || exit 1
($AUTOMAKE --version) >/dev/null 2>/dev/null || (echo "You need GNU automake 1.7 to install from CVS (ftp://ftp.gnu.org/gnu/automake/)"; exit 1) || exit 1

# Determine the version of automake.
automake_version=`$AUTOMAKE --version | head -n 1 | sed -e 's/[^12]*\([12]\.[0-9][^ ]*\).*/\1/'`

# Require automake 1.7.
if expr "1.7" \> "$automake_version" >/dev/null; then
  $AUTOMAKE --version | head -n 1
  echo ""
  echo "Fatal error: automake 1.7 or higher is required.  Please set \$AUTOMAKE"
  echo "to point to a newer automake, or upgrade."
  echo ""
  exit 1
fi

run()
{
  echo "Running $1 ..."
  $1
}

# This is needed when someone just upgraded automake and this cache is still generated by an old version.
rm -rf autom4te.cache config.cache

run "$ACLOCAL"
run "$AUTOCONF"
run "$AUTOMAKE --add-missing --foreign"

