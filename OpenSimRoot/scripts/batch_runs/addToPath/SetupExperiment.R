#!/usr/bin/env Rscript

# author johannes postma
# date 12 july 2017
# license: gpl.v3

# Creates factorial design
 
# simple R script that will create a folder named Output with
# in that folder a list of folders each container input files 
# and executable to run model. Input files are copied
# from the input file folder, and executable from the link
# folder. Input files are changed according to the
# instructions in the Replacement folders
#


#check assumptions
errorExit=function(msg){
     print(msg)
     quit("no",1)
}

here=dir(".")
up=dir("..")
if(sum(here=="job.sh")==0) errorExit("script job.sh is missing")

exename="SimRoot" #todo better to read exe name from job.sh
if(sum(here==exename)==0) exename="OpenSimRoot"
if(sum(here==exename)==0) errorExit("executable SimRoot is missing")
#if(sum(up=="Output")==1) errorExit("Output folder is already there")
if(sum(up=="Replacements")==0) errorExit("Replacements folder is missing")
if(sum(up=="InputFiles")==0) errorExit("InputFiles folder is missing")

#script
require(tools,quietly = T)
setwd("../Replacements")
#print(getwd())
folders=dir()
#collect all the xml files
design=sapply(folders,dir,pattern="*.xml")
#remove empty arrays.
design=design[sapply(design,length)>0]
#remove extensions
design=sapply(design,file_path_sans_ext)
#all combinations
treatmentcombinations=expand.grid(design)
factors=names(treatmentcombinations)
newfoldernames=interaction(treatmentcombinations,sep="_")

print(summary(treatmentcombinations))

#replacement file names
infofiles=paste(factors,"originalPath.txt",sep="/")
targets=sapply(infofiles,readLines,n=1)
names(targets)=factors

problemConstruction=function(foldername,sources,targets){
  #create folder, assuming we are in the output folder
  system(paste("mkdir -p",foldername))
  #copy input files
  system(paste("cp -al ../InputFiles ",foldername))
  #copy executables
  system(paste("cp -al ../bin/",exename," ",foldername,sep=""))
  system(paste("cp -al ../bin/job.sh ",foldername))
  #rm the targets
  targetpaths=paste(foldername,"InputFiles",targets,sep="/")
  #print( paste(targetpaths,collapse=" ") )
  system(paste("rm -f",paste(targetpaths,collapse=" ")))
  #copy the sources to the targets (assuming they have the same length)
  sourcepaths=paste("../Replacements",sources,sep="/")
  dummy=sapply(paste("cp",sourcepaths,targetpaths),system)
}

system("mkdir -p ../Output")
setwd("../Output")

for(i in 1:length(newfoldernames)){
  foldername=as.character(newfoldernames[i])
  sources=paste(names(targets),paste(unlist(treatmentcombinations[i,]),"xml",sep="."),sep="/")
  if(length(sources)!=length(targets)){
     errorExit("error, sources and targets are not the same length")
  }
  problemConstruction(foldername,sources,targets)
}

treatmentcombinations$path=newfoldernames
write.table(treatmentcombinations,file="experimentalDesign.txt",row.names=F)




