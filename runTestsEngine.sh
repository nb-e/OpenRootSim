#!/bin/bash
cd OpenSimRoot/tests/engine

./testEngine.sh "../../StaticBuild/OpenSimRoot"
rex=$?
echo finished testing engine with error status $rex

cd ../../..

exit $rex


