#!/bin/bash
export ANDROID_NDK=/usr/local/src/android-ndk-r19c
rm -r build
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
		-DANDROID_ABI="armeabi-v7a" \
			-DANDROID_NDK=$ANDROID_NDK \
				-DANDROID_PLATFORM=android-22 \
		..
make && make install
cd ..
