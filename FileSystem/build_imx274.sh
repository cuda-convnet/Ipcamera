#bash
rm -rf ../lib/*;
rm -rf makefile.rule
cp makefile_imx274.rule makefile.rule
make clean;
make;

#rm -rf avicodelib;
#rm -rf filesystem;
echo "end";
cp ../lib/* ../../pcnvr_3516e/lib



