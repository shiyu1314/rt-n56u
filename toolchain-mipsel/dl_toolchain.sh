#!/bin/sh

DIR="toolchain-3.4.x"
DL_NAME="mipsel-linux-uclibc.tar.xz"
DL_URL="https://github.com/GH-X/rt-n56u/raw/main/toolchain-mipsel/$DL_NAME"

if [ -d $DIR ]; then
	echo "$DIR exists!"
	exit
fi

if [ -r $DL_NAME ]; then
	mkdir -p $DIR && \
	tar -xvf $DL_NAME -C $DIR
else
	curl -O -L $DL_URL && \
	mkdir -p $DIR && \
	tar -xvf $DL_NAME -C $DIR
fi
