mkdir -p ./out/linux/debug
mkdir -p ./out/linux/release

cd ./out/linux/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../..

cd ../release
cmake -DCMAKE_BUILD_TYPE=Release ../../..
