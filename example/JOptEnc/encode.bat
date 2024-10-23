echo "encode PNG"
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon.png" -o "baboon_png_enc.jpeg" -r "baboon_rec.png" -ps 512x512 -ff PNG -q 90 -v 3

echo "encode BMP"
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon.bmp" -o "baboon_bmp_enc.jpeg" -r "baboon_rec.bmp" -ps 512x512 -ff BMP -q 90 -v 3

echo "encode RAW BMP"
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon_512x512.gbr" -o "baboon_GRR_enc.jpeg" -r "baboon_rec_512x512.gbr" -ps 512x512 -pt GBR -q 90 -v 3

echo "encode planar RAW YCbCr 4:4:4"
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon_512x512_cf444.yuv" -o "baboon_yuv444_enc.jpeg" -r "baboon_512x512_cf444_rec.yuv" -ps 512x512 -cf 444 -q 90 -v 3

echo "encode planar RAW YCbCr 4:2:2"
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon_512x512_cf422.yuv" -o "baboon_yuv422_enc.jpeg" -r "baboon_512x512_cf422_rec.yuv" -ps 512x512 -cf 422 -q 90 -v 3

echo "encode planar RAW YCbCr 4:2:0"
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon_512x512_cf420.yuv" -o "baboon_yuv420_enc.jpeg" -r "baboon_512x512_cf420_rec.yuv" -ps 512x512 -cf 420 -q 90 -v 3

echo "encode with different chroma formats"
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon.png" -o "baboon_png_444_enc.jpeg" -r "baboon_png_444_rec.png" -ps 512x512 -ff PNG -cf 444 -q 90 -v 3
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon.png" -o "baboon_png_422_enc.jpeg" -r "baboon_png_422_rec.png" -ps 512x512 -ff PNG -cf 422 -q 90 -v 3
"../../buildW/JOptEnc/Release/JOptEnc" -i "baboon.png" -o "baboon_png_420_enc.jpeg" -r "baboon_png_420_rec.png" -ps 512x512 -ff PNG -cf 420 -q 90 -v 3
