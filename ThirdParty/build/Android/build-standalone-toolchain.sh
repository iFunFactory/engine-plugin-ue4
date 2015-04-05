
# toolchain for armv7
sudo $NDK_ROOT/build/tools/make-standalone-toolchain.sh \
--ndk-dir=$NDK_ROOT \
--system=darwin-x86_64 --platform=android-9 \
--toolchain=arm-linux-androideabi-4.6 --install-dir=/usr/ndk/armv7

# toolchain for arm64
sudo $NDK_ROOT/build/tools/make-standalone-toolchain.sh \
--ndk-dir=$NDK_ROOT \
--system=darwin-x86_64 --platform=android-21 \
--toolchain=aarch64-linux-android-4.9 --install-dir=/usr/ndk/arm64

# toolchain for x86
sudo $NDK_ROOT/build/tools/make-standalone-toolchain.sh \
--ndk-dir=$NDK_ROOT \
--system=darwin-x86_64 --platform=android-9 \
--toolchain=x86-4.6 --install-dir=/usr/ndk/x86

# toolchain for x64
sudo $NDK_ROOT/build/tools/make-standalone-toolchain.sh \
--ndk-dir=$NDK_ROOT \
--system=darwin-x86_64 --platform=android-21 \
--toolchain=x86_64-4.9 --install-dir=/usr/ndk/x64
