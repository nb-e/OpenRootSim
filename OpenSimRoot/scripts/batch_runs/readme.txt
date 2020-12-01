instruction for running a sensitivity analysis using factorial design

prerequisites:
1) working on linux with bash and R installed
2) ssh access to a linux cluster with torque or SGE installed
3) your ssh key installed on the cluster, so we do not need to type passwords


steps to test:
1) create a folder
2) copy the Replacements folder into your working folder
3) copy the bin folder into your folder
4) copy the repositories InputFiles folder into your working folder
5) add the scripts in the addToPath folder to /usr/local/bin
6) login to cluster
7) pull and build the OpenSimRoot code on the cluster
8) create a scratch folder in your home folder on the cluster. It will contain temporary data
9) logout
10) copy the executable from the cluster to your working folder on your own pc
11) go one folder up and run runOncluster workingfoldername clustername username

If all goes well, things are automatically synced to the cluster and submitted to the cluster. 
The result will be synced back and the postProcessingScript in the bin folder will be run
in order to summarize the results

what to do next:

1) Setup input files to have your default scenario as you like it

2) Create folders in the Replacements folder according to your experimental design. Each factor
should have a folder. Each folder should have the set of alternative parameterizations (levels)
and a OriginalPath.txt which says where this should be copied to. So in short: factors are folders
and levels are files in those folders

3) Added the postPorcessingScript so that you get the output that you want

4) Delete or rename the Output folde

5) run in the parent directory: runOncluster workingfoldername clustername username

6) Check your results when the cluster is done. 




