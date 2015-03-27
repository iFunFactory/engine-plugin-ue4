# Use the tools from the Standalone Toolchain
export PATH=$PATH:/opt/ndk/arm/bin

mkdir -p build/android-armv7

# Run the configure to target a static library for the ARMv7
./Configure --prefix=$(pwd)/build/android-armv7 \
--cross-compile-prefix=arm-linux-androideabi- \
android-armv7 no-shared no-idea no-md2 no-mdc2 no-rc4 no-rc5

# Build
make -j4
make install
