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

echo "=== Configuring x264 ==="

rm -rf "$X264_BUILD"
mkdir -p "$X264_BUILD"
cd "$X264_BUILD"
"$X264_SOURCE/configure" --prefix="$PREFIX" \
  --disable-cli \
  --enable-static

echo "=== Building x264 ==="

make -j$(nproc) all install

echo "=== Configuring ffmpeg ==="

rm -rf "$FFMPEG_BUILD"
mkdir -p "$FFMPEG_BUILD"
cd "$FFMPEG_BUILD"

"$FFMPEG_SOURCE/configure" --prefix="$PREFIX" \
  --disable-programs \
  --disable-encoders \
  --disable-decoders \
  --disable-hwaccels \
  --disable-bsfs \
  --disable-protocols \
  --disable-indevs \
  --disable-outdevs \
  --disable-filters \
  --disable-muxers \
  --disable-demuxers \
  --disable-doc \
  --disable-autodetect \
  --enable-gpl \
  --enable-gpl \
  --enable-libx264 \
  --enable-libx265 \
  --enable-libvpx \
  --enable-encoder=libx264 \
  --enable-decoder=h264 \
  --enable-avcodec \
  --enable-avformat \
  --enable-swscale \
  --enable-protocol=file \
  --enable-bsf=mjpeg2jpeg \
  --enable-muxer=avi --enable-muxer=h264 --enable-muxer=mjpeg --enable-muxer=mp4 \
  --enable-demuxer=h264 --enable-demuxer=m4v --enable-demuxer=mp3 --enable-demuxer=mpegvideo --enable-demuxer=mpegps --enable-demuxer=mjpeg --enable-demuxer=mov --enable-demuxer=avi --enable-demuxer=aac --enable-demuxer=pmp --enable-demuxer=oma --enable-demuxer=pcm_s16le --enable-demuxer=pcm_s8 --enable-demuxer=wav

# --disable-everything
# --enable-parser=h264 --enable-parser=mpeg4video --enable-parser=mpegaudio --enable-parser=mpegvideo --enable-parser=mjpe--enable-parser=aac --enable-parser=aac_latm


#  --enable-encoder=libx265 \
#  --enable-encoder=libvpx_vp9 \
#  --enable-decoder=hevc \
#  --enable-decoder=libvpx_vp9 \

echo "=== Building ffmpeg ==="

make -j$(nproc) all install
