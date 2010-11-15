#!/bin/bash
cd "$(dirname "$0")"
deps_changed=0

function isuptodate {
  [ -f lib/$2 ] && \
    (make -C $1 -q TARGET=i386 2>/dev/null) && \
    (make -C $1 -q TARGET=x86_64 2>/dev/null)
}

function makeuniversal {
  origd="$(pwd)"
  cd $1
  #rm -rf build-*
  CFLAGS="$CFLAGS -arch i386 $3" make TARGET=i386 || exit $?
  CFLAGS="$CFLAGS -arch x86_64 $3" make TARGET=x86_64 || exit $?
  rm -f ../lib/$2
  lipo build-*i386*/$2 build-*x86_64*/$2 -output ../lib/$2 -create
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
makeuniversal_ifdirty libwapcaplet libwapcaplet.a
lipo -info lib/libwapcaplet.a

echo '------------------- libparserutils -------------------'
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
[ ! -d include/parserutils ] && \
  cp -vfR libparserutils/include/parserutils include/
[ ! -d include/libwapcaplet ] && \
  cp -vfR libwapcaplet/include/libwapcaplet include/
cp -vfR libcss/include/libcss include/

echo '------------------- example1 -------------------'

cd libcss/examples
gcc -g -W -Wall -o example1 example1.c -lcss -lparserutils -lwapcaplet -I../../include -L../../lib
./example1

echo '------------------- CSS.framework -------------------'

cd ../../cocoa-framework
xcodebuild -target CSS -parallelizeTargets -configuration Release || exit $?
cp -Rp build/Release/CSS.framework ../lib/CSS.framework

echo "done"