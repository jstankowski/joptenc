echo "convert to BMP"
ffmpeg -y -i baboon.png baboon.bmp

echo "convert to planar RAW RGB (with GBR compnent order - the only planar RGB format allowed by ffmpeg)"
ffmpeg -y -i baboon.png -vf "scale=in_range=full:out_range=full,format=rgb24" -f rawvideo -pix_fmt gbrp baboon_512x512.rgb && ren baboon_512x512.rgb baboon_512x512.gbr

echo "convert to planar RAW YCbCr with 4:4:4 chroma format"
ffmpeg -y -i baboon.png -vf "scale=in_range=full:out_range=full,scale=out_color_matrix=bt601,format=yuv444p" -f rawvideo -pix_fmt yuv444p baboon_512x512_cf444.yuv

echo "convert to planar RAW YCbCr with 4:2:2 chroma format"
ffmpeg -y -i baboon.png -vf "scale=in_range=full:out_range=full,scale=out_color_matrix=bt601,format=yuv422p" -f rawvideo -pix_fmt yuv422p baboon_512x512_cf422.yuv

echo "convert to planar RAW YCbCr with 4:2:0 chroma format"
ffmpeg -y -i baboon.png -vf "scale=in_range=full:out_range=full,scale=out_color_matrix=bt601,format=yuv420p" -f rawvideo -pix_fmt yuv420p baboon_512x512_cf420.yuv
