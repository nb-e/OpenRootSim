#!/bin/bash
timeSeries.R "grep plantDryWeight | grep maize" maizeDryWeight 3 20 40
timeSeries.R "grep rootLength | grep maize" maizeRootLength 3  20 40
timeSeries.R "grep shootDryWeight | grep maize" maizeShootDryWeight 3  20 40
timeSeries.R "grep rootDryWeight | grep maize" maizeRootDryWeight 3  20 40

#add more lines for different moments in time, and add phosphorus....vtu as additional file.
#find ../Output -maxdepth 2 -name "roots040.000.vtp" -execdir ../../bin/vtk2jpeg.py {} \;




