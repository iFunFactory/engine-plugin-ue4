TARGET_OS=x86_64

# Make build directory
rm -rf build/${TARGET_OS}
mkdir -p build/${TARGET_OS}

# Run the configure to target a static library for the x64
./Configure --prefix=$(pwd)/build/${TARGET_OS} \
darwin64-x86_64-cc no-shared no-idea no-md2 no-mdc2 no-rc4 no-rc5

# Build
make -j4
make install
