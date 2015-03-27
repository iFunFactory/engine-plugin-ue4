TARGET_OS=armv7-a
TARGET_CXX=armeabi-v7a
TOOLCHAIN_PATH=/opt/ndk/arm
TARGET_HOST=arm-linux-androideabi

export NDK=/Users/arin/android-ndk-r10d

# Use the tools from the Standalone Toolchain
export PATH=$PATH:$TOOLCHAIN_PATH/bin
export SYSROOT=$TOOLCHAIN_PATH/sysroot
export CC="${TARGET_HOST}-gcc --sysroot $SYSROOT"
export CXX="${TARGET_HOST}-g++ --sysroot $SYSROOT"
export CXXSTL=$NDK/sources/cxx-stl/gnu-libstdc++/4.9

mkdir -p build/${TARGET_OS}

# Run the configure to target a static library for the ARMv7
./configure --prefix=$(pwd)/build/${TARGET_OS} \
--host=$TARGET_HOST \
--with-sysroot=$SYSROOT \
--disable-shared \
--enable-cross-compile \
CFLAGS="-march=${TARGET_OS}" \
CXXFLAGS="-march=${TARGET_OS} -I$CXXSTL/include -I$CXXSTL/libs/${TARGET_CXX}/include"

# Build
make -j4
make install
