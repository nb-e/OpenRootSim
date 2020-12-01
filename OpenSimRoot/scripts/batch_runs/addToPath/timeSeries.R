#!/usr/bin/env Rscript

#check assumptions
errorExit=function(msg){
     print(msg)
     quit("no",1)
}

args=commandArgs(TRUE)
searchString=args[1]
label=args[2]
row=args[3]
if(length(args)>3){  
  timepoints=args[4:length(args)]
}else{
  timepoints=NA
}


setwd("../Output")
filename=paste(label,"tab",sep=".")
if(file.exists(as.character(filename)) ) {
  errorExit(paste("file",filename,"exists"))
}

getdata=function(datafile,searchString,row){
  command=paste("cat",datafile,"| eval",searchString,"| cut -f",2)
  time=system(command,intern=T)
  command=paste("cat",datafile,"| eval",searchString,"| cut -f",row)
  result=system(command,intern=T)
  return(list(time=time,result=result) )
}

design=read.table("experimentalDesign.txt",header=T)
if( file.exists("results.dat") ) {
   ndesign=read.table("results.dat",header=T)
}else{
   ndesign=design
}

results=list()
nrow=0
for(i in 1:length(design$path)){
   folder=as.character(design$path[i])
   datafile=paste(folder,"tabled_output.tab",sep="/")
   if( file.exists(datafile) ) {
     #print(paste("processing",datafile))
     data=getdata(datafile,searchString,row)
     if(length(data$time)>length(results$time)){
       results$time=data$time
       nrow=length(data$time)
       mt=max(as.numeric(data$time))
       timepoints=na.omit(as.numeric(timepoints))
       timepoints=unique(c(timepoints,mt))
     }
     results[[folder]]=data$result
     for(t in na.omit(timepoints)){
       columnname=paste(label,t,sep=".")
       timepoint=data$result[as.numeric(data$time)==as.numeric(t)]
       if(length(timepoint)==1){
          ndesign[i,columnname]=timepoint
       }else{
          if(length(timepoint)>1) {
             timepoint=NA
             print(paste("No unique data (check filter) at time=",t," label=",label," folder=",folder))
          }else{
             timepoint=NA
             print(paste("No data at time=",t," label=",label," folder=",folder))
          }
          ndesign[i,columnname]=timepoint
       }
     }
   }else{
     print(paste("skipping non existing file",datafile))
   }
}

padding=function(v,nrow){
   r=vector("character",length=nrow)
   r[]="NA"
   if(length(v)) r[1:length(v)]=v
   return(r)
}


results2=lapply(results,padding,nrow=nrow)
df=do.call("cbind",results2)

write.table(df,file=filename,row.names=F,quote=F)
write.table(ndesign,file="results.dat",row.names=F,quote=F)




