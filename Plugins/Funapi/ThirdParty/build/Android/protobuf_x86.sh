export PATH=/usr/ndk/x86/bin:$PATH
export SYSROOT=$NDK_ROOT/platform/android-9/arch-x86
export CC=i686-linux-android-gcc
export CXX=i686-linux-android-g++
export OUTPUT_DIR=$(pwd)/build/x86

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

make distclean

./autogen.sh
./configure --enable-static --disable-shared \
--host=i686-linux-android \
--with-sysroot=$SYSROOT CC=$CC CXX=$CXX \
--enable-cross-compile \
--prefix=$OUTPUT_DIR \
--with-protoc=protoc \
LIBS="-lc"

make -j4
make install