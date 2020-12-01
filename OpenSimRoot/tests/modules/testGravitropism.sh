#!/bin/bash
echo Testing gravitropism

mkdir -p testResults
cd testResults

echo Testing RootGravitropism.xml 
if [ -z "$1" ] 
then
exe="../../Release/OpenSimRoot"
else
exe="$1"
fi

#[ -e ../$exe ] && echo using $exe as exe || { echo exe $exe not found ; exit 1 ; }

../$exe ../RootGravitropism.xml  > screen.out 2>&1 && echo "Test RootGravitropism.xml passed"  || echo "Test RootGravitropism.xml failed"
mv tabled_output.tab RootGravitropism.tab  2>/dev/null
grep -v "Simulation took (hours:minutes:seconds)" warnings.txt &> RootGravitropism.txt 
mv roots010.00.vtu roots010.00.RootGravitropism.vtu  2>/dev/null
mv roots010.00.vtp roots010.00.RootGravitropism.vtp  2>/dev/null

rm -f *0.vtu
rm -f *0.vtp
rm -f *0.vti
rm -f *.pvd
rm -f warnings.txt
cd ..

echo Done running gravitropism test, comparing results
#no use as mem use always differs somewhat
#diff -q testResults/screen.out refTestResults/screen.out 

diff -q refTestResults/RootGravitropism.tab testResults/RootGravitropism.tab
VTPDifference=$(Rscript ./ParseXML.R ./refTestResults/roots010.00.RootGravitropism.vtp ./testResults/roots010.00.RootGravitropism.vtp)
VTPDifference="${VTPDifference:4:${#VTPDifference}}"
if (( VTPDifference < 0 )); then
	echo "roots010.00.RootGravitropism.vtp contains a different amount of points"
	rexe=$(($rexe+1))
fi
if (( VTPDifference > 0 )); then
	echo "$VTPDifference coordinates in the VTP files differ by more than 0.1%"
	rexe=$(($rexe+1))
fi
VTUDifference=$(Rscript ./ParseXML.R ./refTestResults/roots010.00.RootGravitropism.vtu ./testResults/roots010.00.RootGravitropism.vtu)
VTUDifference="${VTUDifference:4:${#VTUDifference}}"
if (( VTUDifference < 0 )); then
	echo "roots010.00.RootGravitropism.vtu contains a different amount of points"
	rexe=$(($rexe+1))
fi
if (( VTUDifference > 0 )); then
	echo "$VTUDifference coordinates in the VTU files differ by more than 0.1%"
	rexe=$(($rexe+1))
fi
exit $rexe
