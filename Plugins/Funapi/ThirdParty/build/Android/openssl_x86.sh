# Use the tools from the Standalone Toolchain
export PATH=$PATH:/opt/ndk/x86/bin

mkdir -p build/android-x86

# Run the configure to target a static library for the x86
./Configure --prefix=$(pwd)/build/android-x86 \
--cross-compile-prefix=i686-linux-android- \
android-x86 no-shared no-idea no-md2 no-mdc2 no-rc4 no-rc5

# Build
make -j4
make install
