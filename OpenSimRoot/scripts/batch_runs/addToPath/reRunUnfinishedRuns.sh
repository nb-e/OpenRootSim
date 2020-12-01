#!/bin/bash
if [ ${PWD##*/} = "bin" ]; then
#if [ -d "../bin" ]; then
  path2output="../Output"
else
  path2output="."
fi

if [ "$1" == "" ] ; then
  echo "Usage: reRunUnfinishedRuns.sh time options"
  echo "*time an integer number corresponding to the date that you want the runs to run to i.e. 40"
  echo "*options: -interactive, -showoutput, or -runall"
  echo "no options will default to listing the runs that stopped before 'time'"
  exit
fi

if [ $1 -eq $1 2> /dev/null ] ; then
  endtime=$1
else
  echo "Error: first argument must be numeric, indicating time. Run without arguments to see help"
  exit
fi

if [  ! -d $path2output ]; then 
  echo "Error: Output folder with runs not found"
  exit
fi

function showoutput {
    outputfile=`ls -t $1/*.o | head -n1`
    echo outputfile is $outputfile in $1
    if [ -e "$outputfile" ] 
    then
      cat $outputfile
      echo -e "\033[0m"
      echo ""
    fi
}

function showoutputpagewise {
    outputfile=`ls -t $1/*.o | head -n1`
    echo outputfile is $outputfile in $1
    if [ -e "$outputfile" ] 
    then
      cat $outputfile | less
      echo -e "\033[0m"
      echo ""
    fi
}

function rerun {
    cd $1
#    echo "Warning: not syncing any files - turned this off in the script"
    rsync ../../bin/SimRoot ./  2> /dev/null
    rsync ../../bin/job.sh ./  2> /dev/null
    rsync ../../InputFiles/simulationControlParameters.xml ./InputFiles 2> /dev/null
    label=rr_`basename $1`
    #command -v qsub >/dev/null &&  echo ./job.sh &&  echo 
    command -v qsub >/dev/null 2>&1 || { echo >&2 "qsub required but it's not installed.  Aborting."; exit 1; }
    qsub -N $label job.sh
    cd ..
}
 
#set mode
if [ "$2" == "-interactive" ] ; then
   show="i"
   function action {
      if [ $2 != -1 ] ; then
         read -p "Found $1   which ended on day $1. Show output? (y/n)"
         if [ "$REPLY" == "y" ] ; then
           showoutput $1 $2
         fi
      fi
      read -p "restart $1    which ended on day $2 (y/n)?"
      if [ "$REPLY" == "y" ] ; then
          rerun $1 $2
      fi
   }
elif [ "$2" == "-showoutput" ] ; then
   show="a"
   function action {
      if [ $2 != -1 ] ; then
         echo  "Output of $1 which ended on day $2."
         showoutputpagewise $1 $2
      fi
    }
 elif [ "$2" == "-showoutputall" ] ; then
   show="a"
   function action {
      if [ $2 != -1 ] ; then
         echo  "Output of $1 which ended on day $2."
         showoutput $1 $2
      fi
    }
elif [ "$2" == "-runall" ] ; then
   show="r"
   function action {
      rerun $1 $2
   }
else
   show="y"   
   function action {
      if [ $2 == -1 ] ; then
          echo -e "\t$1      which did not run."
      else    
          echo -e "\t$1      which ended on day $2."
      fi
   }
fi

#go to wd dir
pushd $path2output

#create list of all unfinished jobs
   list=`find ./ -name "job.sh" -exec dirname {} \;`
   cnt=0;
   for i in $list ; do
      datafile="$i/tabled_output.tab"
      if [ -f $datafile ]; then  
        numblines=`cat $datafile | wc -l`

        if [ $numblines -gt 1 ]; then 
          lc=`tail -n1 $datafile | cut -f2`
          lk=${lc/.*}
        else
          lk=0
        fi
        #echo datafile is $datafile numblines is $numblines and lk is $lk

        if [ $endtime -gt $lk ]; then
           #job did not run til the required time
           dirlist[cnt]=$i
           datelist[cnt]=$lk
           cnt=`expr $cnt + 1`
        fi
      else
        #job did not run/produce output
        dirlist[cnt]=$i
        datelist[cnt]=-1
        cnt=`expr $cnt + 1`
      fi 
   done   

l=${#dirlist[@]}
echo "Found $l uncompleted jobs."

#run action function for each of the jobs
for ((i = 0; i < $l ; i++)); do
  action ${dirlist[$i]} ${datelist[$i]}
  cnt=`expr $cnt + 1`
done

#write help instructions
if [ $show == "y" ];then   
  echo  "To run or view output of these runs use one of the options: -interactive, -showoutput, -showoutputall or -runall "
fi

#return from wd dir
pushd > /dev/null



