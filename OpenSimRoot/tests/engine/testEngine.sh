#!/bin/bash
echo Testing engine

mkdir -p testResults
cd testResults
if [ -z "$1" ] 
then
exe="../../Release/OpenSimRoot"
else
exe="$1"
fi

[ -e ../$exe ] && echo using $exe as exe || { echo exe $exe not found ; exit 1 ; }


../$exe ../SimulaConstant.xml > screen.out 2>&1 || echo "Test SimulaConstant.xml failed" && echo "Test SimulaConstant.xml passed"  
mv tabled_output.tab ResultSimulaConstant.tab 2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsSimulaConstant.txt 

../$exe ../SimulaTable.xml >> screen.out 2>&1 || echo "Test SimulaTable.xml failed" && echo "Test SimulaTable.xml passed"  
[ -e /usr/bin/Rscripts ] && cat tabled_output.tab | grep -v path | ../../../scripts/plot -qo SimulaTable
mv tabled_output.tab ResultSimulaTable.tab 2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsSimulaTable.txt 

../$exe ../SimulaGrid.xml >> screen.out 2>&1 || echo "Test SimulaGrid.xml failed" && echo "Test SimulaGrid.xml passed"  
mv tabled_output.tab ResultSimulaGrid.tab 2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsSimulaGrid.txt

../$exe ../SimulaVariable.xml >> screen.out 2>&1  || echo "Test SimulaVariable.xml failed" && echo "Test SimulaVariable.xml passed"   
[ -e /usr/bin/Rscripts ] && cat tabled_output.tab | grep RGRModel | grep -v multi | grep -v Rate | ../../../scripts/plot -qo SimulaVariable
mv tabled_output.tab ResultSimulaVariable.tab 2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsSimulaVariable.txt

../$exe ../SimulaPoint.xml >> screen.out 2>&1  || echo "Test SimulaPoint.xml failed" && echo "Test SimulaPoint.xml passed"  
mv modelDump010.00.xml ResultSimulaPoint.tab 2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsSimulaPoint.txt 

../$exe ../SimulaStochastic.xml >> screen.out 2>&1  || echo "Test SimulaStochastic.xml failed" && echo "Test SimulaStochastic.xml passed"  
[ -e /usr/bin/Rscripts ] && ../SimulaStochastic.R
mv tabled_output.tab ResultSimulaStochastic.tab 2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> WarningsSimulaStochastic.txt 

rm -f *.xml
rm -r warnings.txt
cd ..

echo Done running tests, comparing results
#no use as mem use always differs somewhat
#diff -q testResults/screen.out refTestResults/screen.out 
find testResults -name "*.tab"  -type f -exec sh -c 'diff -q {} refTestResults/$(basename {})' \;
rexe=$?
find testResults -name "*.txt"  -type f -exec sh -c 'diff -q {} refTestResults/$(basename {})' \;
rexe=$(($?+$rexe))
echo Done
exit $rexe

