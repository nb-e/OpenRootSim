#!/bin/bash
#Eclipse auto generates the make files for the project
#this folder contains a copy of the make files for a release 
#build. This is intended for building OpenSimRoot without 
#eclipse. To maintain this folder, please do not edit manually
#but simply use this code. 

cd ../Release  
make clean
cd ..
rsync -a --delete --exclude "readme.txt" Release/* StaticBuild
cd StaticBuild
#find . -type f -name "*.mk" -execdir sed -i 's,src/,OpenSimRoot/src/,g' {} \;
#find . -type f -name "makefile" -execdir sed -i 's,src/,OpenSimRoot/src/,g' {} \;

