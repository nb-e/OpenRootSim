#!/bin/bash
./cleanup.sh
cd ./OpenSimRoot/StaticBuild/
./readme.txt
cd ../..
cd ./OpenSimRoot/StaticBuild_win64/
./readme.txt
cd ../..
./build.sh
./runTests.sh
./cleanup.sh

