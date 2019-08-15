#bash
rm -rf ../lib/*;
rm -rf makefile.rule
cp makefile_imx290_lvds2.rule makefile.rule
make clean;
make;

#rm -rf avicodelib;
#rm -rf filesystem;
echo "end";
cp ../lib/* ../../ipc3519v101/lib
cp ../lib/*.so /nfs/imx290_lvds


