rm -rf build/
mkdir build
cd build
#cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TEST=ON
