#!/bin/bash
cd OpenSimRoot/StaticBuild_win64
make clean
make -j4 all 
re=$?
cd ../..
exit $re 

