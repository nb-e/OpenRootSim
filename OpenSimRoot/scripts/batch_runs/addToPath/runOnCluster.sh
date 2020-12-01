#!/bin/bash
#this script is used to submit a run to the cluster
# first argument should be folder name which has default layout (bin,Replacements,InputFiles,Output)
if [ "$1" == "" ] ; then
  echo "Usage: runOncluster.sh foldername clustername username"
  echo "foldername is the name of the folder that contains the bin, Replacements and Inputfiles folders"
  echo "clustername is optional. Default is lionxi.rcc.psu.edu"
  exit
fi
runFolder=`basename $1`
cluster=$2
if [ "$cluster" == "" ] ; then
  cluster="greenmrx"
fi

userName="$3"
if [ "$userName" == "" ] ; then
  userName=`who am i | nawk '{print $1}'`
fi

#this is used for rerunning jobs.
endtime="40"

#use pid as 'unique' id
id=$$

#function for syncing data
syncdataback(){
  echo "syncing data back to $runFolder" 
  echo "rsync -aHz  --rsh='ssh -l $userName' --exclude=.svn  $cluster:$remoteFolder/Output/ $runFolder/Output"
  rsync -aHz  --rsh="ssh -l $userName" --exclude=.svn $cluster:$remoteFolder/Output/ $runFolder/Output
}

#function to check the running jobs on the cluster
checkRunningJobs(){
   shortun=${userName:0:7}
   if [ "$clustertype" == "SGE" ]
   then
   nrunning=$((`ssh -l $userName $cluster "qstat -u $userName -s r | grep $shortun | wc -l"`))
   nqueued=$((`ssh -l $userName $cluster "qstat -u $userName -s p | grep $shortun |  wc -l"`))
   ntotal=$((`ssh -l $userName $cluster "qstat -u $userName -s prs  | grep $shortun |  wc -l"`))
   currentTime=`date`
   echo At $currentTime $ntotal jobs in queue of which $nrunning are running and $nqueued are queued
   else
   nrunning=$((`ssh -l $userName $cluster "qstat -u $userName | grep $shortun  | grep -w R | wc -l"`))
   nqueued=$((`ssh -l $userName $cluster "qstat -u $userName | grep $shortun | grep -w Q | wc -l"`))
   ntotal=$((`ssh -l $userName $cluster "qstat -u $userName  | grep $shortun | grep -vw C | wc -l"`))
   currentTime=`date`
   echo At $currentTime $ntotal jobs in queue of which $nrunning are running and $nqueued are queued
   fi
}


#function for deleting jobs from cluster
killJobs() {
  checkRunningJobs
  #clear input
  while read -r -t 0; do read -r; done
  echo "Interupted, do you want to sync data from cluster? 'y', all other keys is no";
  read answer
  if [ "$answer" == "y" ] ; then
    syncdataback
  else
    echo "Not syncing data back, for manual sync use:"
    echo "rsync -aHz  --rsh='ssh -l $userName' --exclude=.svn  $cluster:$remoteFolder/Output/ $runFolder/Output"
  fi
  echo "Deleting all jobs from cluster!";
  ssh -l $userName $cluster "qdel $clusterIDs" > /dev/null 2>&1
  exit
}


#step 1 : send data to cluster
#check folder structure
checkFolder(){
  if [ ! -d "$1" ]; then
    echo "$1 not found."
    exit
  fi
}
checkFile(){
  if [ ! -f "$1" ]; then
    echo "$1 not found."
    exit
  fi
}


checkFolder "$runFolder/bin"
checkFile "$runFolder/bin/job.sh"
checkFolder "$runFolder/InputFiles"
checkFolder "$runFolder/Replacements"

mkdir -p "$runFolder/Output"

#info message
echo "using folder $runFolder. Will run it on $cluster with userName $userName. Press enter to continue or ctrl+c to abort"
read

#cluster type
if ssh  -l $userName $cluster stat /mnt/cluster/gridEngine/ \> /dev/null 2\>\&1
            then
                    echo This is SGE cluster
		    clustertype="SGE"
            else
                    echo Assuming this is a torque cluster
                    clustertype="torque"
fi

#rsync
template=${runFolder}"_XXXXXX"
backupFolder=`ssh -l $userName $cluster "mktemp -t $template -u -d -p ~/scratch/"`
remoteFolder="~/scratch/${runFolder}"
echo $`ssh -l $userName $cluster "if [ -d "${remoteFolder}" ]; then echo \"Backing up on server $remoteFolder $backupFolder\" ; mv $remoteFolder $backupFolder; fi;"`

echo "syncing folder $runFolder to $cluster:$remoteFolder"
if [ $? -ne 0 ]; then
   echo "Failed to create a remote folder. Exit"
   exit
fi   
rsync -aHz --delete --rsh="ssh -l $userName" --exclude=.svn $runFolder/bin $cluster:$remoteFolder
rsync -aHz --delete --rsh="ssh -l $userName" --exclude=.svn $runFolder/InputFiles $cluster:$remoteFolder
rsync -aHz --delete --rsh="ssh -l $userName" --exclude=.svn $runFolder/Replacements $cluster:$remoteFolder
#check exit status
if [ $? -ne 0 ]; then
   echo "Rsync failed. Exit"
   exit
fi   

#step 2 : submit jobs to cluster
echo "submitting jobs to cluster"
trap killJobs INT


if [ "$(ls -A $runFolder/Output)" ]; then
    echo "Output $path2output is not Empty assuming we are rerunning the runs"
else
    echo "Creating Output folder using SetupExperiment.R script"
    pushd "$runFolder/bin"
    /usr/local/bin/SetupExperiment.R
    pushd
fi 

echo "Syncing the reRunUnfinishedRuns.sh script"
rsync /usr/local/bin/reRunUnfinishedRuns.sh "$runFolder/bin"
echo "Syncing the Output folder"
rsync -aHz  --exclude="*.vt*" --exclude="*.raw*" --exclude=.svn --delete --rsh="ssh -l $userName" $runFolder/Output $cluster:$remoteFolder
sleep 5 #allow the cluster filesystem to write buffers
echo "running the reRunUnfinshedRuns.sh $endtime -runall"
clusterIDs=`ssh -l $userName $cluster "cd $remoteFolder/bin;./reRunUnfinishedRuns.sh $endtime -runall | grep $cluster | tr '\n' ' '"`
echo "These runs submitted to the queue: $clusterIDs"

#wait for cluster to finish
echo "waiting for jobs on cluster to be finished"
sleep 20s
ntotal=10
checkRunningJobs
if [ $ntotal -gt 0 ]; then
  sleep 1m
fi
checkRunningJobs
if [ $ntotal -gt 0 ]; then
  sleep 5m
fi
while [ $ntotal -gt 0 ]; do
   checkRunningJobs
   #echo "syncing partial data back to $runFolder"
   #rsync -aHz  --rsh="\'ssh -l $userName\'" $cluster:$remoteFolder/Output/ $runFolder/Output
   if [ $ntotal -gt 0 ]; then
     sleep 20m
   fi;
done
trap - INT

#step 3 : sync data back to hort-ana
syncdataback

#step 4 : optional post run processing
echo "running post data processing scripts"
cd $runFolder/bin/
find ../Output -maxdepth 1 -name "*.tab" -execdir mv {} {}.old \;
find ../Output -maxdepth 1 -name "lastlines*.dat" -execdir mv {} {}.old \;
./postProcessingScript.sh 

echo runs have completed. Results are in $runFolder/Output






