#!/bin/bash
cd OpenSimRoot/StaticBuild
make clean > /dev/null
cd ../..
cd OpenSimRoot/StaticBuild_win64
make clean > /dev/null
cd ../..
rm -rf OpenSimRoot/tests/engine/testResults
rm -rf OpenSimRoot/tests/modules/testResults

