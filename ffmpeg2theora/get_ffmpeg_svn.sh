#!/bin/sh

#optional, if you have those libs installed:
#extra="--enable-libfaad --enable-libgsm --enable-amr_nb --enable-amr_nb-fixed --enable-amr_wb --enable-amr_if2"

common="--enable-gpl --enable-swscaler --enable-pp --enable-liba52 --disable-encoders --enable-libvorbis"

#linux
options="$common --enable-pthreads $extra"

#mingw32
uname | grep MINGW && options="$common --enable-memalign-hack --enable-mingw32 --extra-cflags=-I/usr/local/include --extra-ldflags=-L/usr/local/lib $extra"

svn co -r11076 svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg
cd ffmpeg && ./configure $options && make