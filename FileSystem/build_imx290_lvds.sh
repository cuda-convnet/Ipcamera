#bash
rm -rf ../lib/*;
rm -rf makefile.rule
cp makefile_imx290_lvds.rule makefile.rule

make;

#rm -rf avicodelib;
#rm -rf filesystem;
echo "end";
cp ../lib/* ../../pcnvr_3516e/lib



