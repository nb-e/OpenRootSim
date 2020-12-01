#!/usr/bin/env Rscript

#
#  script looks for a folder with vtu files and produces for each fem . vtu  
#  a summary of an data array.
#
#
#
#

args <- commandArgs(trailingOnly = TRUE)

#get script arguments
  getArgument<-function(argn,defaultsetting){
    n= 1+c(1:length(args))[args==argn]
    if(length(n)){
      args[n]
    }else{
      defaultsetting
    }
  }

#print help and exit
printhelp<-function(){
  cat(
    "\
R script for reducing 3d fem.vtu dat to 1 d arrays. Example usage:\
\
1) femvtu2profile.R myfolder
\
additional arguments behind plotting command:\
  -h -help --help   this help\
  -o                outputfilename (default summarytable.xls)\
  -s                stepsize (default 1 cm)\
  -d                data array (default theta)\
\
"
  )
}
#print help
h=sum(args=="-h")+sum(args=="--help")+sum(args=="-help")
if(h | length(args)==0){
  printhelp()
  quit('no')
}


require(XML)

folder=args[length(args)]
filenames=dir(folder,pattern = "^fem.......vtu")
outputfilename=getArgument("-o","summarytable.xls")
stepSize=as.numeric(getArgument("-s","1"))
dataarrayname=getArgument("-d","theta")


getDepthProfile<-function(filename,dataarrayname="theta",FUN=mean,stepSize=1.){
  print(paste("processing",filename))
  ifile=xmlParse(filename)
  
  #header
  #<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian">
  #<UnstructuredGrid>
  #  <Piece NumberOfPoints="5687" NumberOfCells="4600">
  #  <Points>
  #  <DataArray type="Float64" Name="Position" NumberOfComponents="3" format="ascii">
  
  xpCoords = xpathApply(ifile,"//VTKFile[@type='UnstructuredGrid']/UnstructuredGrid/Piece/Points/DataArray[@Name='Position']")
  if(xmlSize(xpCoords)!=1) stop("Did not find the Coordinates")
  
  
  #coords
  coordschar=unlist(strsplit(xmlValue(xpCoords[[1]]),split="[[:space:]]"))
  coordinates=as.numeric(coordschar)
  #double spaces lead to empty entries, which can be removed
  #coordschar[which(is.na(coordinates))]
  coordinates=na.exclude(coordinates)
  
  #declared length of array
  xpPiece = xpathApply(ifile,"//VTKFile[@type='UnstructuredGrid']/UnstructuredGrid/Piece")
  l=as.numeric(xmlAttrs(xpPiece[[1]])[["NumberOfPoints"]])
  #check that we found all
  if(l!=length(coordinates)/3.) stop("length of coordinates != declared length in piece")
  
  #array of interest
  datapath=paste("//VTKFile[@type='UnstructuredGrid']/UnstructuredGrid/Piece/PointData/DataArray[@Name='",dataarrayname,"']",sep="")
  xpData = xpathApply(ifile,datapath)
  if(xmlSize(xpData)!=1) stop(paste("Did not find the array name",dataarrayname))
  datachar=unlist(strsplit(xmlValue(xpData[[1]]),split="[[:space:]]"))
  datanum=as.numeric(datachar)
  datanum=na.exclude(datanum)
  if(l!=length(datanum)) stop("length of data != declared length in piece")
  
  #aggregate data along y coordinates
  #ycoordinates
  y=coordinates[((1:l)*3)-1]
  #round off on 1 cm values
  yfac=as.factor(stepSize*floor(y/stepSize))
  sumd=aggregate(datanum,by=list(depth=yfac),FUN)
  names(sumd)[2]=dataarrayname
  return(sumd)
}

d=getDepthProfile(paste(folder,filenames[1],sep="/"),dataarrayname = dataarrayname, stepSize = stepSize)
names(d)[2]=filenames[1]
for(filename in filenames[2:length(filenames)]){
  res=try(getDepthProfile(paste(folder,filename,sep="/"),dataarrayname = dataarrayname, stepSize = stepSize)[,2])
  if (class(res) != "try-error")  d[filename]=res
}
write.table(x = d, file = outputfilename, col.names = T, row.names = F)



