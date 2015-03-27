TARGET_OS=armv8-a
TARGET_CXX=arm64-v8a
TOOLCHAIN_PATH=/opt/ndk/aarch64
TARGET_HOST=aarch64-linux-android

export NDK=/Users/arin/android-ndk-r10d

# Use the tools from the Standalone Toolchain
export PATH=$PATH:$TOOLCHAIN_PATH/bin
export SYSROOT=$TOOLCHAIN_PATH/sysroot
export CC="${TARGET_HOST}-gcc --sysroot $SYSROOT"
export CXX="${TARGET_HOST}-g++ --sysroot $SYSROOT"
export CXXSTL=$NDK/sources/cxx-stl/gnu-libstdc++/4.9

mkdir -p build/${TARGET_OS}

# Run the configure to target a static library for the ARMv8
./configure --prefix=$(pwd)/build/${TARGET_OS} \
--host=$TARGET_HOST \
--with-protoc=protoc \
--with-sysroot=$SYSROOT \
--disable-shared \
--enable-cross-compile \
CFLAGS="-march=${TARGET_OS}" \
CXXFLAGS="-march=${TARGET_OS} -I$CXXSTL/include -I$CXXSTL/libs/${TARGET_CXX}/include"

# Build
make -j4
make install
