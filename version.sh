#!/bin/sh
# Create/Update version.h
# Written by Georgi Chorbadzhiyski

VERSION=$(cat RELEASE 2>/dev/null || echo 'unknown')
GIT_VER=$(git describe --tags --dirty --always 2>/dev/null || echo 'release')

TMPFILE=`mktemp /tmp/version.h.XXXXXX` || exit 1

trap "{ rm -f $TMPFILE ; exit 1 ; }" INT TERM

echo "\
#ifndef _VERSION_H_
#define _VERSION_H_

#define VERSION \"${VERSION}\"
#define GIT_VER \"${GIT_VER}\"
#define PROGRAM_NAME \"videohubctrl\"

#endif\
" > $TMPFILE

if ! cmp version.h $TMPFILE >/dev/null 2>/dev/null
then
	cat $TMPFILE > version.h
	echo "Updated version.h with VERSION=${VERSION} GIT_VER=${GIT_VER}"
else
	echo "version.h is up to date with VERSION=${VERSION} GIT_VER=${GIT_VER}"
fi

rm -f $TMPFILE
