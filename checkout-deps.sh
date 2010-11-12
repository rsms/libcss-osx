#!/bin/bash
cd "$(dirname "$0")"

if [ ! -d libparserutils ]; then
  svn co svn://svn.netsurf-browser.org/trunk/libparserutils
fi

if [ ! -d libwapcaplet ]; then
  svn co svn://svn.netsurf-browser.org/trunk/libwapcaplet
fi
