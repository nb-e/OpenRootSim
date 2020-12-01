#!/usr/bin/env Rscript

suppressMessages(require("XML"))

file2="testResults/ResultWaterTestSchnepf.vtp.xml"

xf2= xmlParseDoc(file2)

coordinates2 <- scan ( text = xpathSApply(xf2, "//DataArray[@Name='nextxyz']", xmlValue), quiet = T)
n2=length(coordinates2)/3.
y2=coordinates2[((1:n2)*3)-1]
psi_x2 <- scan ( text = xpathSApply(xf2, "//DataArray[@Name='xylemWaterPotential']", xmlValue), quiet = T)

#remove root tip, which is not solved
psi_x2[n2]=NA

#analytical solution
g = 1000. #9.81 # gravitational acceleration (m/s^2)   
rho =  1.e3 # 997 density of water, (kg/m^3)      
ref = 0.#1.e5 # reference pressure (kg/ (m s^2))
L = 0.5                # length of single straight root (m)
a = 2.e-3              # radius (m)
kz = 5.e-13            # axial conductivity (m^5 s / kg) (mal rho ergibt die alten einheiten)
kr = 2.e-9             # radial conductivity per root type (m^2 s / kg) 

toPa<-function(ph){ # cm pressure head to Pascal (kg/ (m s^2))
  return (ref + ph / 100. * rho * g) }

toHead<-function(pa){ # Pascal (kg/ (m s^2)) to cm pressure head
  return ((pa-ref) * 100 / rho / g)}

p_s = toPa(-200)       # static soil pressure (cm) 
p0 = toPa(-1000)       # dircichlet bc at top
c = 2*a*pi*kr/kz
#AA = np.array([  [1,1],    [sqrt(c)*exp(sqrt(c)*(-L)), -sqrt(c)*exp(-sqrt(c)*(-L))]   ]) # dirichlet top, neumann bot
v1= sqrt(c)*exp( sqrt(c)*(-L))
v2=-sqrt(c)*exp(-sqrt(c)*(-L))
AA=matrix(c(1,1,v1,v2),2,2,byrow = T)
bb = c(p0-p_s, -rho*g) 
d = solve(AA, bb) # compute constants d_1 and d_2 from bc

p_r <-function(z){
  return (toHead( p_s + d[1]*exp(sqrt(c)*z) + d[2]*exp(-sqrt(c)*z) ))
}
y3=(-50:0)


#plot
svg(filename = "testResults/waterUptakeText.svg")
plot(psi_x2,y2, xlim=c(-1000,-200))
lines(p_r(y3/100.),y3)
invisible(dev.off())

#compare
error=sqrt(sum((p_r(y2/100.)-psi_x2)^2,na.rm = T)/n2)
if(error>5.) {
  print("WaterUptakeTest: Schnepf code deviates on average more than 5 hPa.") 
  quit(save="no",status=1)
}else{
  print("WaterUptakeTest: Schnepf code is close to analytical solution. Test passed.") 
  quit(save="no",status=0)
}

