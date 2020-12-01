#!/bin/bash
echo Testing modules

mkdir -p testResults
cd testResults

echo Testing OneSimpleStraightRoot.xml 
if [ -z "$1" ] 
then
exe="../../Release/OpenSimRoot"
else
exe="$1"
fi

[ -e ../$exe ] && echo using $exe as exe || { echo exe $exe not found ; exit 1 ; }

../$exe ../OneSimpleStraightRoot.xml > screen.out 2>&1 && echo "Test OneSimpleStraightRoot.xml passed"  || echo "Test OneSimpleStraightRoot.xml failed"
mv tabled_output.tab ResultOneSimpleStraightRoot.tab  2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsOneSimpleStraightRoot.txt 
mv roots010.00.vtu roots010.00.OneSimpleStraightRoot.vtu  2>/dev/null
mv roots010.00.vtp roots010.00.OneSimpleStraightRoot.vtp  2>/dev/null

../$exe ../PartiallyPredefinedBranchedRoot.xml >> screen.out 2>&1 && echo "Test PartiallyPredefinedBranchedRoot.xml passed" || echo "Test PartiallyPredefinedBranchedRoot.xml failed"
#[ -e /usr/bin/Rscripts ] && cat tabled_output.tab | grep -v path | ../../../scripts/plot -qo PartiallyPredefinedBranchedRoot
mv tabled_output.tab ResultPartiallyPredefinedBranchedRoot.xml.tab  2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsPartiallyPredefinedBranchedRoot.xml.txt  
#vti run, but output not in default test as it is 7 mb or so. 
#mv roots_rasterImage_014.00.vti ResultPartiallyPredefinedBranchedRoot.vti 
mv roots014.00.vtp ResultPartiallyPredefinedBranchedRoot.xml.vtp  2>/dev/null

../$exe ../Barber-Cushman.xml >> screen.out 2>&1 && echo "Test Barber-Cushman.xml passed" || echo "Test Barber-Cushman.xml failed"
mv tabled_output.tab ResultBarber-Cushman.xml.tab  2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsBarber-Cushman.xml.txt  

../$exe ../SimpleCropModel.xml >> screen.out 2>&1 && echo "Test SimpleCropModel.xml passed" || echo "Test SimpleCropModel.xml failed"
mv tabled_output.tab ResultSimpleCropModel.xml.tab  2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsSimpleCropModel.xml.txt  

../$exe ../straightRootSchnepf.xml >> screen.out 2>&1 && echo "Simulation StraighRootSchnepf ran" || echo "Simulation StraighRootSchnepf failed"
mv roots020.00.vtp ResultWaterTestSchnepf.vtp.xml  2>/dev/null

rm -f *0.vtu
rm -f *0.vtp
rm -f *0.vti
rm -f *.pvd
rm -f warnings.txt
cd ..

./testGravitropism.sh "$exe"
rexe=$?
./testWaterUptakeSchnepf.R  
rexe=$(($?+$rexe))

echo Done running testing modules, comparing results
#no use as mem use always differs somewhat
#diff -q testResults/screen.out refTestResults/screen.out 

find testResults -name "*.tab"  -type f -exec sh -c 'diff -q {} refTestResults/$(basename {})' \;
rexe=$(($?+$rexe))
find testResults -name "*.txt"  -type f -exec sh -c 'diff -q {} refTestResults/$(basename {})' \;
rexe=$(($?+$rexe))
find testResults -name "*.vtu"  -type f -exec sh -c 'diff -q {} refTestResults/$(basename {})' \;
rexe=$(($?+$rexe))
find testResults -name "*.vtp"  -type f -exec sh -c 'diff -q {} refTestResults/$(basename {})' \;
rexe=$(($?+$rexe))

#find testResults -name "*.vti" -execdir diff -q {} ../refTestResults/{} \;
echo Done exiting with code $rexe
exit $rexe

