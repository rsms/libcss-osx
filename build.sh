#!/bin/bash
cd "$(dirname "$0")"

[ "$1" == "--force" ] && (./reset.sh || exit $?)

# --------------------------------------------------------------------------
# checkout

NETSURF_SVN_REV=11123
if [ ! -d libparserutils ]; then
  if ! (svn co svn://svn.netsurf-browser.org/trunk/libparserutils@$NETSURF_SVN_REV) ; then
    S=$?
    rm -rf libparserutils
    exit $S
  fi
fi
if [ ! -d libwapcaplet ]; then
  if ! (svn co svn://svn.netsurf-browser.org/trunk/libwapcaplet@$NETSURF_SVN_REV) ; then
    S=$?
    rm -rf libparserutils
    exit $S
  fi
fi
if [ ! -d libcss ]; then
  if ! (svn co svn://svn.netsurf-browser.org/trunk/libcss@$NETSURF_SVN_REV) ; then
    S=$?
    rm -rf libparserutils
    exit $S
  fi
  # apply patches
  for patchfile in patches/libcss/*.patch; do
    patch -p0 -d libcss --backup-if-mismatch -i "../$patchfile" || exit $?
  done
fi

# --------------------------------------------------------------------------

export PKG_CONFIG_PATH="$(pwd)/libparserutils:$(pwd)/libwapcaplet:$PKG_CONFIG_PATH"
deps_changed=0

function isuptodate {
  [ -f lib/$2 ] && \
    (make -C $1 -q TARGET=i386 2>/dev/null) && \
    (make -C $1 -q TARGET=x86_64 2>/dev/null)
}

function makeuniversal {
  origd="$(pwd)"
  cd $1
  CFLAGS="$CFLAGS -arch i386 $3" make TARGET=i386 || exit $?
  CFLAGS="$CFLAGS -arch x86_64 $3" make TARGET=x86_64 || exit $?
  rm -f ../lib/$2
  lipo build-*i386*/$2 build-*x86_64*/$2 -output ../lib/$2 -create
  ranlib ../lib/$2
  cd "$origd"
}

function makeuniversal_ifdirty {
  if ! (isuptodate $1 $2); then
    makeuniversal $1 $2
    deps_changed=1
  fi
}

mkdir -p lib

echo '------------------- libwapcaplet -------------------'
ln -fs ../libwapcaplet.pc libwapcaplet/libwapcaplet.pc
makeuniversal_ifdirty libwapcaplet libwapcaplet.a
lipo -info lib/libwapcaplet.a

echo '------------------- libparserutils -------------------'
ln -fs ../libparserutils.pc libparserutils/libparserutils.pc
makeuniversal_ifdirty libparserutils libparserutils.a
lipo -info lib/libparserutils.a

echo '------------------- libcss -------------------'
if ! (isuptodate libcss libcss.a) || [ "$deps_changed" = "1" ]; then
  makeuniversal libcss libcss.a \
      '-I../libparserutils/include -I../libwapcaplet/include -L..'
fi
lipo -info lib/libcss.a

echo '------------------- headers -------------------'
mkdir -p include

echo cp -fR libparserutils/include/parserutils include/
rm -rf include/parserutils
cp -fR libparserutils/include/parserutils include/

echo cp -fR libwapcaplet/include/libwapcaplet include/
rm -rf include/libwapcaplet
cp -fR libwapcaplet/include/libwapcaplet include/

echo cp -fR libcss/include/libcss include/
rm -rf include/libcss
cp -fR libcss/include/libcss include/

find include -name .svn -type d -exec rm -rf '{}' ';' 2>/dev/null

echo '------------------- example1 -------------------'

cd libcss/examples
gcc -g -W -Wall -o example1 example1.c \
  -lcss -lparserutils -lwapcaplet -L../../lib -I../../include -L../../lib \
  || exit $?
./example1

#echo '------------------- CSS.framework -------------------'
#
#cd ../../cocoa-framework
#xcodebuild -project CSS.xcodeproj \
#           -target CSS \
#           -parallelizeTargets \
#            -configuration Release \
#            build \
#            > /dev/null || exit $?
# 
# rm -rf ../lib/CSS.framework
# cp -Rp build/Release/CSS.framework ../lib/CSS.framework
# 
# echo "done"
