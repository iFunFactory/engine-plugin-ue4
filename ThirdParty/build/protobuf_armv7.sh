export PATH=/usr/ndk/armv7/bin:$PATH
export SYSROOT=$NDK_ROOT/platform/android-9/arch-arm
export CC=arm-linux-androideabi-gcc
export CXX=arm-linux-androideabi-g++
export OUTPUT_DIR=$(pwd)/build/armv7

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

make distclean

./autogen.sh
./configure --enable-static --disable-shared \
--host=arm-linux-androideabi \
--with-sysroot=$SYSROOT CC=$CC CXX=$CXX \
--enable-cross-compile \
--prefix=$OUTPUT_DIR \
--with-protoc=protoc \
LIBS="-lc"

make -j4
make install