#!/bin/sh

OSTYPE=`uname -s`
MACHINE=`uname -m`
AUTO_MAKE_VERSION=`automake --version | head -1 | awk '{print $4}' | sed -e 's/\-p[0-9]$//' | sed -e 's/\.//'`

echo "Host info: $OSTYPE $MACHINE"
echo -n " automake: `automake --version | head -1 | awk '{print $4}'`"
echo " ($AUTO_MAKE_VERSION)"
echo ""

ACLOCAL_OPTS=""
if [ $AUTO_MAKE_VERSION -ge 14 ]; then
    if [ $OSTYPE = "IRIX" -o $OSTYPE = "IRIX64" ]; then    echo " -I ."
        ACLOCAL_OPTS="-I ."
    fi
fi
echo "Running aclocal $ACLOCAL_OPTS"
aclocal $ACLOCAL_OPTS 2> autogen.err

echo "Running autoheader"
autoheader 2>> autogen.err
if [ ! -e simgear/simgear_config.h.in ]; then
    echo "ERROR: autoheader didn't create simgear/simgear_config.h.in!"
    echo "Aborting ... consulte autogen.err for details."
    exit 1
fi    

echo -n "Running automake"
if [ $OSTYPE = "IRIX" -o $OSTYPE = "IRIX64" ]; then
    echo " --add-missing --include-deps"
    automake --add-missing --include-deps 2>> autogen.err
else
    echo " --add-missing"
    automake --add-missing 2>> autogen.err
fi

echo "Running autoconf"
autoconf 2>> autogen.err

if [ ! -e configure ]; then
    echo "ERROR: configure was not created!"
    echo "Aborting ... consulte autogen.err for details."
    exit 1
fi

# fixup Makefiles for Irix
if test "$OSTYPE" = "IRIX" -o "$OSTYPE" = "IRIX64"; then
    echo "Fixing Makefiles for Irix"
    for n in `find . -name Makefile.in`; do \
        mv -f $n $n.ar-new; \
        sed 's/$(AR) cru /$(AR) -o /g' $n.ar-new > $n; \
        rm -f $n.ar-new; \
    done;
fi

echo ""
echo "======================================"

if [ -f config.cache ]; then
    echo "config.cache exists.  Removing the config.cache file will force"
    echo "the ./configure script to rerun all it's tests rather than using"
    echo "the previously cached values."
    echo ""
fi

echo "Now you are ready to run './configure'"
echo "======================================"

rm -f autogen.err
