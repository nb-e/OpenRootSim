#!/bin/bash
cd OpenSimRoot/StaticBuild
make clean
make -j4 all 
re=$?
cd ../..
exit $re 

