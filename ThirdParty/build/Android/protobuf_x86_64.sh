export PATH=/usr/ndk/x64/bin:$PATH
export SYSROOT=$NDK_ROOT/platform/android-21/arch-x86_64
export CC=x86_64-linux-android-gcc
export CXX=x86_64-linux-android-g++
export OUTPUT_DIR=$(pwd)/build/x64

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

make distclean

./autogen.sh
./configure --enable-static --disable-shared \
--host=x86_64-linux-android \
--with-sysroot=$SYSROOT CC=$CC CXX=$CXX \
--enable-cross-compile \
--prefix=$OUTPUT_DIR \
--with-protoc=protoc \
LIBS="-lc"

make -j4
make install