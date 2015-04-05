TARGET_OS=x86_64

# Make build directory
rm -rf build/${TARGET_OS}
mkdir -p build/${TARGET_OS}

# Run the configure to target a static library for the x64
./configure \
--prefix=$(pwd)/build/${TARGET_OS} \
--disable-shared \
CFLAGS="-arch x86_64"

# Build
make -j4
make install