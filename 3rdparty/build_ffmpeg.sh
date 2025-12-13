# $0: Install prefix
# $1, $2: FFmpeg source, build folder
# $3, $4: x264 source, build folder

PREFIX="$1"
FFMPEG_SOURCE="$2"
FFMPEG_BUILD="$3"
X264_SOURCE="$4"
X264_BUILD="$5"

set -e

rm -rf "$PREFIX"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"

header() {
  echo -e "\e[33;1m<ffmpeg> $1\e[37;3m ===================================\e[0m"
}

header "Configuring x264"

rm -rf "$X264_BUILD"
mkdir -p "$X264_BUILD"
cd "$X264_BUILD"
"$X264_SOURCE/configure" --prefix="$PREFIX" \
  --disable-cli \
  --enable-static \
  --extra-cflags="-Os -ffunction-sections"

header "Building x264"

make -j$(nproc) all install

header "Configuring ffmpeg"

rm -rf "$FFMPEG_BUILD"
mkdir -p "$FFMPEG_BUILD"
cd "$FFMPEG_BUILD"

"$FFMPEG_SOURCE/configure" --prefix="$PREFIX" \
  --disable-everything \
  --disable-programs \
  --disable-doc \
  --disable-autodetect \
  --enable-small \
  --extra-cflags="-ffunction-sections" \
  --enable-gpl \
  --enable-avcodec \
  --enable-avformat \
  --enable-swscale \
  --enable-protocol=file \
  --enable-libx264 \
  --enable-encoder=libx264 \
  --enable-decoder=h264 \
  --enable-muxer=avi --enable-muxer=h264 --enable-muxer=mjpeg \
    --enable-muxer=mp4 \
  --enable-demuxer=h264 --enable-demuxer=m4v --enable-demuxer=mp3 \
    --enable-demuxer=mpegvideo --enable-demuxer=mjpeg --enable-demuxer=avi \
    --enable-demuxer=aac --enable-demuxer=wav \
  --enable-parser=h264 --enable-parser=mpeg4video --enable-parser=mpegaudio \
    --enable-parser=mpegvideo --enable-parser=aac --enable-parser=aac_latm

#  --enable-libx265 \
#  --enable-libvpx \
#  --enable-encoder=libx265 \
#  --enable-encoder=libvpx_vp9 \
#  --enable-decoder=hevc \
#  --enable-decoder=libvpx_vp9 \

header "Building ffmpeg"

make -j$(nproc) all install
