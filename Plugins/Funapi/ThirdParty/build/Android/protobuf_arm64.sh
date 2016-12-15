export PATH=/usr/ndk/arm64/bin:$PATH
export SYSROOT=$NDK_ROOT/platform/android-21/arch-arm64
export CC=aarch64-linux-android-gcc
export CXX=aarch64-linux-android-g++
export OUTPUT_DIR=$(pwd)/build/arm64

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

make distclean

./autogen.sh
./configure --enable-static --disable-shared \
--host=aarch64-linux-android \
--with-sysroot=$SYSROOT CC=$CC CXX=$CXX \
--enable-cross-compile \
--prefix=$OUTPUT_DIR \
--with-protoc=protoc \
LIBS="-lc"

make -j4
make install