#!/bin/bash
cd OpenSimRoot/tests/engine
./testEngine.sh "../../StaticBuild/OpenSimRoot"
rexe=$?
echo finished testing engine with error status $rexe
cd ../modules
./testModules.sh "../../StaticBuild/OpenSimRoot"
rexm=$?
echo finished testing modules with error status $rexm
./testGravitropism.sh "../../StaticBuild/OpenSimRoot"
rexg=$?
echo finished testing gravitropism with error status $rexg
cd ../../..
rex=$(($rexe+$rexm+$rexg))
echo exiting with error status $rex
exit $rex


