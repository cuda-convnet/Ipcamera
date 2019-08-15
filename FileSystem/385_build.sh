#bash
rm -rf ../lib/*;
rm -rf makefile.rule
cp makefile_imx385.rule makefile.rule
make clean;
make;

#rm -rf avicodelib;
#rm -rf filesystem;
echo "end";

pwd

echo "cp lib";
cp ../lib/* ../../ipc3519v101/lib
cp ../lib/*.so /nfs/imx385

echo "cp ok";



