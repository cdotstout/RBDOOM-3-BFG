cd ..
rm -rf build-vulkan
mkdir build-vulkan
cd build-vulkan
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DVULKAN=TRUE -DFFMPEG=FALSE ../neo
