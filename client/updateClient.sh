# Copy hirecs client
cp -v ../../hirecs/hirecs.cbp .
cp -v ../../hirecs/main.cpp .
cp -vr ../../hirecs/include .
cp -vr ../../hirecs/src .
mkdir -p bin/Release
cp -v  ../../hirecs/bin/Release/hirecs bin/Release/
