#bash
rm -rf ../lib/*;
rm -rf makefile.rule
cp makefile_imx290.rule makefile.rule

make;

#rm -rf avicodelib;
#rm -rf filesystem;
echo "end";


