# Use the tools from the Standalone Toolchain
export PATH=$PATH:/opt/ndk/x86_64/bin

mkdir -p build/android-x64

# Run the configure to target a static library for the x64
./Configure --prefix=$(pwd)/build/android-x64 \
--cross-compile-prefix=x86_64-linux-android- \
android no-shared no-idea no-md2 no-mdc2 no-rc4 no-rc5

# Build
#make -j4
#make install
