
### Build Instructions

first you need to build elf2rpl:
make -C ext/wiiu/elf2rpl

make sure you have the latest ppsspp-ffmpeg submodule:
cd ffmpeg; git pull origin master

then create a build directory, then navigate to it and run:
cmake path/to/ppsspp -DCMAKE_TOOLCHAIN_FILE=path/to/ppsspp/cmake/Toolchains/wiiu.cmake

cmake fails to generate the correct flags the first time so we force it to parse the build files a second time:
touch path/to/ppsspp/CMakeLists.txt

make

if you need console output over network, set the PC_DEVELOPMENT_IP_ADDRESS define in ext/wiiu/logger.c and you can use the ext/wiiu/net_listen.sh script to listen to the log.
the run.sh script be uused to both send the compiled rpx to the HBL and listen to console output.


