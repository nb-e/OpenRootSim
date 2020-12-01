#!/bin/bash
# This is a sample PBS script. It will request 1 processor on 1 node
# for 4 hours.
#
#$ -M j.postma@fz-juelich.de
#
#   Request 1 processors on 1 node
#
#PBS -l nodes=1:ppn=1
#this does not work if the parallel environment is not created $ -pe mpi 1
#
#   Request x hours of walltime, 32 GB of ram
#
#PBS -l walltime=24:00:00
#$ -l h_rt=24:00:00
#
#NOTUSED  PBS -q bigmem
#NOTUSeD  $ -q bigmem
#
#   Request that regular output and terminal output go to the same file
#
#PBS -j oe
#$ -cwd
#$ -j y
#$ -o out.$JOB_ID.o
#$ -e err.$JOB_ID.e
#
#number of nodes on sge cluster
#$ -pe smp 1
#$ -R y
#
# if you want a special queue
###NU$ -q BigMem.q
###NU$ -P BigMem.p
#
# if the cluster has limits on h_vmem set, and opensimroot needs more memory
#$ -l h_vmem=6G
#
#   The following is the body of the script. By default, 
#   PBS scripts execute in your home directory, not the 
#   directory from which they were submitted. The following
#   line places you in the directory from which the job
#   was submitted.
#
if [ $PBS_O_WORKDIR ] 
 then 
 
  cd $PBS_O_WORKDIR
fi
if [ $SGE_O_WORKDIR ]
  then
    cd $SGE_O_WORKDIR
fi
#
#   Now we want to run the program "hello".  "hello" is in
#   the directory that this script is being submitted from,
#   $PBS_O_WORKDIR.
#
wd=`pwd`
echo " "
echo "working dir = $wd "
echo "Job started on `hostname` at `date`"
#./SimRootPlabipd -f InputFiles/runNotSoSimpleSWMS3DExample.xml 
./OpenSimRoot -f InputFiles/main-testing.xml
echo " "
echo "Job Ended at `date`"
echo " "

