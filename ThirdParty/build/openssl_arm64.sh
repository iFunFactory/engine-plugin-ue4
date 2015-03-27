# Use the tools from the Standalone Toolchain
export PATH=$PATH:/opt/ndk/aarch64/bin

mkdir -p build/android-arm64

# Run the configure to target a static library for the ARM64
./Configure --prefix=$(pwd)/build/android-arm64 \
--cross-compile-prefix=aarch64-linux-android- \
android no-shared no-idea no-md2 no-mdc2 no-rc4 no-rc5

# Build
make -j4
make install
