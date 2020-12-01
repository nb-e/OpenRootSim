#!/usr/bin/env python

import vtk
import sys
import os

#print "started"

#read vtu file
reader = vtk.vtkXMLGenericDataObjectReader()
reader2 = vtk.vtkXMLGenericDataObjectReader()
filename=os.path.basename(str(sys.argv[1]))
pfilename=filename
if len(sys.argv)>2: 
   pfilename=str(sys.argv[2])

label=os.getcwd()
#replace . to write to current wd. 
ofilename=label+"."+filename + ".jpg"
if len(sys.argv)>3: 
   ofilename=str(sys.argv[3])

print "processing:",filename,pfilename,"writing it to",ofilename
reader.SetFileName(filename)
reader2.SetFileName(pfilename)
reader.Update()
reader2.Update()

#Extract surface
sF=vtk.vtkDataSetSurfaceFilter()
sF.SetInputConnection(reader.GetOutputPort())
mapper0=vtk.vtkPolyDataMapper()
mapper0.SetInputConnection(sF.GetOutputPort())
sF2=vtk.vtkDataSetSurfaceFilter()
sF2.SetInputConnection(reader2.GetOutputPort())
mapper2=vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(sF2.GetOutputPort())

#create actor
actor0=vtk.vtkActor() 
actor0.SetMapper(mapper0)
#actor0.SetPosition(0 40 20)
actor2=vtk.vtkActor() 
actor2.SetMapper(mapper2)
actor2.GetProperty().SetOpacity(0.4)

# create a rendering window and renderer
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetOffScreenRendering(1)
renWin.AddRenderer(ren)
renWin.SetSize(100,1600)

# assign our actor to the renderer
ren.AddActor(actor0)
ren.AddActor(actor2)

#camera position
camera=vtk.vtkCamera()
#camera.SetClippingRange( 14.411, 14411)
camera.SetFocalPoint( 0, -45, 0)
camera.SetPosition( -200, -45, 0)
#camera.SetViewUp( -0.0301976, 0.359864, 0.932516)
#camera.SetViewAngle( 30)
ren.SetActiveCamera(camera)

#renWin is now rendered to an image
w2i=vtk.vtkWindowToImageFilter()
w2i.SetInput(renWin)
#w2i.SetMagnification(2)
w2i.Update()

#image is written to disk
img=vtk.vtkJPEGWriter()
img.SetInputConnection(w2i.GetOutputPort())
img.SetFileName(ofilename)
img.Write()

#print "finished"

