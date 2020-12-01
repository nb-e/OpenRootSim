#!/usr/bin/env Rscript
#| Copyright © 2016 Forschungszentrum Jülich GmbH
#| All rights reserved.
#|
#| Redistribution and use in source and binary forms, with or without modification, are permitted under the GNU General Public License v3 and provided that the following conditions are met:
#| 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#| 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#| 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#|
#| Disclaimer
#| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#|
#| You should have received the GNU GENERAL PUBLIC LICENSE v3 with this file in license.txt but can also be found at http://www.gnu.org/licenses/gpl-3.0.en.html
source("../barber-cushman-onlineVersion0.31-function-library.R")

simroot=read.table("barber-cushman_dataPoint00000_root.tab")
m_simroot=t(as.matrix(simroot)[,-1])

simroot_exp=read.table("barber_cushman_explicit.tab")
m_simroot_exp=t(as.matrix(simroot_exp)[,-1])

dim=dim(m_simroot)
dim_exp=dim(m_simroot_exp)
#sum(dim-dim_exp)

par_simroot=paramRoothairPublication1983
par_simroot$dr=NULL
par_simroot$k=dim[1]
par_simroot$r1=0.3
par_simroot$endtime=30
par_simroot$dt=par_simroot$endtime/(dim[2]-1)

par_simroot$lh=0.03
par_simroot$Imax=0.0864/par_simroot$tcfactor
par_simroot$Imaxh=0.0864/par_simroot$tcfactor
par_simroot$De=2e-9
par_simroot$Cli=par_simroot$Cli

param=par_simroot #hack, as barber seems to have a bug when you put this straight into the function call
m_Rcode=barber(param,"Running Barber-CushmanModel using R code")

svg("Rcode.svg",height=8,width=12)
plotbarber(par_simroot, m_Rcode)
k=dev.off()

svg("OpenSimRoot_cranknicolson.svg",height=8,width=12)
plotbarber(par_simroot, m_simroot)
k=dev.off()

svg("OpenSimRoot_explicit.svg",height=8,width=12)
plotbarber(par_simroot, m_simroot_exp)
k=dev.off()

