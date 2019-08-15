rm -rf ../lib/*

mkdir ../lib/6x16filesystem
make -f f6x16
mv ../lib/*.so ../lib/6x16filesystem

mkdir ../lib/8x08filesystem
make -f f8x08
mv ../lib/*.so ../lib/8x08filesystem

mkdir ../lib/8x16filesystem
make -f f8x16
mv ../lib/*.so ../lib/8x16filesystem

mkdir ../lib/8108filesystem
make -f f8208
mv ../lib/*.so ../lib/8108filesystem

mkdir ../lib/8116filesystem
make -f f8216
mv ../lib/*.so ../lib/8116filesystem

echo "end"