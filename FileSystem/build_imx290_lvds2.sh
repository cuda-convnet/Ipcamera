#bash
rm -rf ../lib/*;
rm -rf makefile.rule
cp makefile_imx290_lvds2.rule makefile.rule
make clean;
make;

#rm -rf avicodelib;
#rm -rf filesystem;
echo "end";
cp ../lib/* ../../pcnvr_3516e/lib
cp ../lib/*.so /nfs/imx290_lvds


