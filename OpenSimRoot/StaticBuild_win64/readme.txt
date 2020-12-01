#!/bin/bash
#Eclipse auto generates the make files for the project
#this folder contains a copy of the make files for a release 
#build. This is intended for building OpenSimRoot without 
#eclipse. To maintain this folder, please do not edit manually
#but simply use this code. 

cd ../windows  
make clean
cd ..
rsync -a --delete --exclude "readme.txt" windows/* StaticBuild_win64
cd StaticBuild_win64
#find . -type f -name "*.mk" -execdir sed -i 's,src/,OpenSimRoot/src/,g' {} \;
#find . -type f -name "makefile" -execdir sed -i 's,src/,OpenSimRoot/src/,g' {} \;

