#!/usr/bin/env Rscript
suppressMessages(require(XML))
args = commandArgs(trailingOnly=TRUE)
trim <- function (x) gsub("^\\s+|\\s+$", "", x)
Parser <- function(input){
	test = xmlParseDoc(args[1])
	node = xpathApply(test, "//DataArray[@Name='Position']")
	string1 = trim(as.character(xmlToDataFrame(node[length(node)])[1]$text))
	list1 = strsplit(string1, split="[\t,\n]+")
	list1 = as.numeric(list1[[1]])
	test = xmlParseDoc(args[2])
	node = xpathApply(test, "//DataArray[@Name='Position']")
	string2 = trim(as.character(xmlToDataFrame(node[length(node)])[1]$text))
	list2 = strsplit(string2, split="[\t,\n]+")
	list2 = as.numeric(list2[[1]])
	if (length(list1) != length(list2)){
	  return(-1)
	}
	difference = 0
	for(i in 1:length(list1)){
	  test = 0.
	  if (list1[[i]] == 0.){
	    test = abs(list1[[i]] - list2[[i]])
	  }
	  else{
	    test = abs((list1[[i]] - list2[[i]])/list1[[i]])
	  }
	  if (test > 0.001){
	    difference = difference + 1
	  }
	}
	return(difference)
}
Parser(args)