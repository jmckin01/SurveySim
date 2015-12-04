#!/usr/bin/env python

import os
import sys
import datetime
import time

sys.path.append('/usr/local/surveysim/python')

from ModelFile import *


if(len(sys.argv) < 4):
    print "Calling Sequence: "+sys.argv[0]+" field(0=COSMOS,1=SWIRE,2=COSMOS+MIPS) model(0=onlySFG,1=agn,2=composites,3=cold, 4=SFG_cold) lfForm(0=MS,1=DPL,2=S)"
    quit()
else:
    field=int(sys.argv[1])
    model=int(sys.argv[2])
    lfForm=int(sys.argv[3])

#initialize
mod=ModelFile()
mod.axis1="ColorF1F2"
mod.axis2="Flux1"

#run settings; should be same regardless of model
mod.settings['verbosity']=3
mod.annealing['temp']=.03
mod.annealing['learningRate']=0.4
mod.convergence['CI']=0.90

mod.filters[0].setID("SPIRE_250")
mod.filters[1].setID("SPIRE_350")
mod.filters[2].setID("SPIRE_500")

#set parameters for field, and input file
if(field == 0):
    simname="spire_COSMOS"
    obsfile="/usr/local/surveysim/obs/L2-COSMOS_xID250_DR2.fits"
    mod.survey['area']=4.78
    mod.filters[0].limit=8.0
    mod.filters[0].err=1.6
    mod.filters[0].compN=1.48
    mod.filters[0].compB=5.90
    mod.filters[0].compM=9.27
    mod.filters[1].limit=0.1
    mod.filters[1].err=1.32
    #mod.filters[1].compN=1.48
    #mod.filters[1].compB=4.84
    #mod.filters[1].compM=5.51
    mod.filters[2].limit=0.1
    mod.filters[2].err=1.9
elif(field == 1):
    simname="spire_Lockman-SWIRE"
    obsfile="/usr/local/surveysim/obs/L5-Lockman-SWIRE_xID250_DR2.fits"
    mod.survey['area']=15.31
    mod.filters[0].limit=9.6
    mod.filters[0].err=1.92
    mod.filters[0].compN=3.80
    mod.filters[0].compB=14.06
    mod.filters[0].compM=12.72
    mod.filters[1].limit=0.1
    mod.filters[1].err=1.58
    #mod.filters[1].compN=22.58
    #mod.filters[1].compB=13.72
    #mod.filters[1].compM=-19.83
    mod.filters[2].limit=0.1
    mod.filters[2].err=2.3
elif(field == 2):
    simname="spire_mips_COSMOS"
    obsfile="/usr/local/surveysim/obs/L2-COSMOS_xID24_DR3.fits"
    #load limits and filters from pre-made model file
    mod.load("/usr/local/surveysim/model/spire_mips_model.fits")

    mod.filters[2].setID("SPIRE_250")
    mod.filters[1].limit=6.0
    mod.filters[2].limit=6.0

#<<<<<<< HEAD
#    mod.filters[0].compN=2110.4
#    mod.filters[0].compB=0.111
#    mod.filters[0].compM=-0.894

    mod.filters[0].err=12.0 #assume 5sigma limit on the 24um
    mod.filters[1].err=3.0 #assume 250um 1sigma confusion
    mod.filters[2].err=3.0 #assume 250um 1sigma confusion
#=======
#<<<<<<< HEAD
#    # mod.filters[1].limit=0.1
#    # mod.filters[2].limit=0.1
    mod.filters[0].compN=2110.4
    mod.filters[0].compB=0.111
    mod.filters[0].compM=-0.894
#=======
#    mod.filters[0].compN=1.0
#    mod.filters[0].compB=0.01
#    mod.filters[0].compM=0.08
#>>>>>>> origin/master
#>>>>>>> 9e961357219762991788e8d314c93394199dcecf
else:
    raise ValueError("Invalid field")

print mod.survey['area']

#default to all fit
mod.fitAllParams()

#remove color evolution
mod.params['cexp'].value=0
mod.params['cexp'].fixed=1
mod.params['zbc'].value=0
mod.params['zbc'].fixed=1

#set luminosity function form
if(lfForm == 0):
    mod.setLF("ModifiedSchecter")
    simname=simname+"_MS"
elif(lfForm == 1):
    mod.setLF("DoublePowerLaw")
    simname=simname+"_DPL"
elif(lfForm == 2):
    mod.setLF("Schechter")
    simname=simname+"_S"

else:
    raise ValueError("Invalid lfForm")

mod.params['fa0'].value=0.60
mod.params['fa0'].pmin=0.20
mod.params['fa0'].pmax=1.00
mod.params['fa0'].fixed=0

mod.params['t1'].value=-1.30
mod.params['t1'].pmin=-2.60
mod.params['t1'].pmax=-0.10
mod.params['t1'].fixed=0

mod.params['t2'].value=6.50
mod.params['t2'].pmin=4.50
mod.params['t2'].pmax=8.50
mod.params['t2'].fixed=0

mod.params['zbt'].value=2.50
mod.params['zbt'].pmin=0.80
mod.params['zbt'].pmax=4.00
mod.params['zbt'].fixed=0

#AS-change
mod.params['fa0'].pmin=0.0
mod.params['zbt'].pmin=0.50
mod.params['zbt'].pmax=4.0
mod.params['t2'].pmin=0.0
mod.params['t1'].pmax=0.5

if(model == 0):
    simname=simname+"_onlySFG"
    fixKeys=['fa0','zbt','t1','t2','fcomp','fcold']
    mod.params['fa0'].value=0
    mod.params['zbt'].value=0
    mod.params['t1'].value=0
    mod.params['t2'].value=0
    mod.params['fcomp'].value=0
    mod.params['fcold'].value=0
elif(model == 1):
    simname=simname+"_agnOnly"
    fixKeys=['fcomp','fcold']
    mod.params['fcold'].value=0
    mod.params['fcomp'].value=0
elif(model == 2):
    simname=simname+"_fComp"
    fixKeys=['fcold']
    mod.params['fcold'].value=0
    mod.params['fcomp'].value=0.5
    mod.params['fcomp'].pmin=0.01
    mod.params['fcomp'].pmax=0.99
elif(model == 3):
    simname=simname+"_fCompfCold"
    fixKeys=[]
    mod.params['fcold'].value=0.5
    mod.params['fcold'].pmin=0.01
    mod.params['fcold'].pmax=0.99
    mod.params['fcomp'].value=0.5
    mod.params['fcomp'].pmin=0.01
    mod.params['fcomp'].pmax=0.99
elif(model == 4):
    simname=simname+"_SFG_cold"
    fixKeys=['fa0','zbt','t1','t2','fcomp']
    mod.params['fa0'].value=0
    mod.params['zbt'].value=0
    mod.params['t1'].value=0
    mod.params['t2'].value=0
    mod.params['fcomp'].value=0
    mod.params['fcold'].value=0.5
    mod.params['fcold'].pmin=0.01
    mod.params['fcold'].pmax=0.99
else:
    raise ValueError("Invalid model")

for key in fixKeys:
    mod.params[key].fixed=1


if(model == 0 and lfForm == 2 and field == 0):
    simname="A-C"
if(model == 0 and lfForm == 1 and field == 0):
    simname="B-C"
if(model == 0 and lfForm == 0 and field == 0):
    simname="C-C"
if(model == 1 and lfForm == 2 and field == 0):
    simname="D-C"
if(model == 1 and lfForm == 1 and field == 0):
    simname="E-C"
if(model == 1 and lfForm == 0 and field == 0):
    simname="F-C"
if(model == 2 and lfForm == 2 and field == 0):
    simname="G-C"
if(model == 2 and lfForm == 1 and field == 0):
    simname="H-C"
if(model == 2 and lfForm == 0 and field == 0):
    simname="I-C"
if(model == 3 and lfForm == 2 and field == 0):
    simname="J-C"
if(model == 3 and lfForm == 1 and field == 0):
    simname="K-C"
if(model == 3 and lfForm == 0 and field == 0):
    simname="L-C"
if(model == 4 and lfForm == 2 and field == 0):
    simname="M-C"
if(model == 4 and lfForm == 1 and field == 0):
    simname="N-C"
if(model == 4 and lfForm == 0 and field == 0):
    simname="O-C"
    
if(model == 0 and lfForm == 2 and field == 1):
    simname="A-LS"
if(model == 0 and lfForm == 1 and field == 1):
    simname="B-LS"
if(model == 0 and lfForm == 0 and field == 1):
    simname="C-LS"
if(model == 1 and lfForm == 2 and field == 1):
    simname="D-LS"
if(model == 1 and lfForm == 1 and field == 1):
    simname="E-LS"
if(model == 1 and lfForm == 0 and field == 1):
    simname="F-LS"
if(model == 2 and lfForm == 2 and field == 1):
    simname="G-LS"
if(model == 2 and lfForm == 1 and field == 1):
    simname="H-LS"
if(model == 2 and lfForm == 0 and field == 1):
    simname="I-LS"
if(model == 3 and lfForm == 2 and field == 1):
    simname="J-LS"
if(model == 3 and lfForm == 1 and field == 1):
    simname="K-LS"
if(model == 3 and lfForm == 0 and field == 1):
    simname="L-LS"
if(model == 4 and lfForm == 2 and field == 1):
    simname="M-LS"
if(model == 4 and lfForm == 1 and field == 1):
    simname="N-LS"
if(model == 4 and lfForm == 0 and field == 1):
    simname="O-LS"
    
if(model == 0 and lfForm == 2 and field == 2):
    simname="A"
if(model == 0 and lfForm == 1 and field == 2):
    simname="B"
if(model == 0 and lfForm == 0 and field == 2):
    simname="C"
if(model == 1 and lfForm == 2 and field == 2):
    simname="D"
if(model == 1 and lfForm == 1 and field == 2):
    simname="E"
if(model == 1 and lfForm == 0 and field == 2):
    simname="F"
if(model == 2 and lfForm == 2 and field == 2):
    simname="G"
if(model == 2 and lfForm == 1 and field == 2):
    simname="H"
if(model == 2 and lfForm == 0 and field == 2):
    simname="I"
if(model == 3 and lfForm == 2 and field == 2):
    simname="J"
if(model == 3 and lfForm == 1 and field == 2):
    simname="K"
if(model == 3 and lfForm == 0 and field == 2):
    simname="L"
if(model == 4 and lfForm == 2 and field == 2):
    simname="M"
if(model == 4 and lfForm == 1 and field == 2):
    simname="N"
if(model == 4 and lfForm == 0 and field == 2):
    simname="O"


mfile=simname+"test_model.fits"
outfile=simname+"test_output.fits"


#parameters below should be the same regardless of model
#mod.params['Alpha'].value=3.00
#mod.params['Alpha'].pmin=0.80
#mod.params['Alpha'].pmax=3.20
#mod.params['Alpha'].fixed=0

#AS-change
mod.params['Alpha'].value=1.2
mod.params['Alpha'].fixed=1

#mod.params['Beta'].value=0.52
#mod.params['Beta'].pmin=0.01
#mod.params['Beta'].pmax=0.55
#mod.params['Beta'].fixed=0

#AS-change
mod.params['Beta'].value=0.5
mod.params['Beta'].fixed=1

#mod.params['Phi0'].value=-2.239
#mod.params['Phi0'].pmin=-3.239
#mod.params['Phi0'].pmax=-0.739
#mod.params['Phi0'].fixed=0

mod.params['Phi0'].value=-2.7
mod.params['Phi0'].pmin=-2.8
mod.params['Phi0'].pmax=-2.6
mod.params['Phi0'].fixed=0

mod.params['L0'].value=9.949
mod.params['L0'].pmin=8.449
mod.params['L0'].pmax=10.400
mod.params['L0'].fixed=0

#AS-change
#mod.params['L0'].pmin=9.0
#mod.params['L0'].pmax=11.0

mod.params['P'].value=-0.57
mod.params['P'].pmin=-1.00
mod.params['P'].pmax=-0.01
mod.params['P'].fixed=0


#AS-change
mod.params['P'].pmin=-1.0
mod.params['P'].max=-0.13

mod.params['P2'].value=-2.40
mod.params['P2'].pmin=-3.00
mod.params['P2'].pmax=1.60
mod.params['P2'].fixed=0

#AS-change
#mod.params['P2'].pmin=-4
#mod.params['P2'].pmax=2

mod.params['Q'].value=3.55
#mod.params['Q'].pmin=3.05
#mod.params['Q'].pmax=5.50
#mod.params['Q'].fixed=0

#AS-change
mod.params['Q'].pmin=3.35
mod.params['Q'].pmax=3.75

#mod.params['Q'].value=3.55
#mod.params['Q'].pmin=2.84#3.05
#mod.params['Q'].pmax=4.26#4.2
mod.params['Q'].fixed=0

#mod.params['Q2'].value=0.80
#mod.params['Q2'].pmin=-2.20
#mod.params['Q2'].pmax=1.50
#mod.params['Q2'].fixed=0

mod.params['Q2'].value=0.80
mod.params['Q2'].pmin=0.64#0.10
mod.params['Q2'].pmax=0.96#1.50
mod.params['Q2'].fixed=0

mod.params['zbp'].value=1.00
mod.params['zbp'].pmin=0.8
mod.params['zbp'].pmax=1.5
mod.params['zbp'].fixed=0

#MB-change
#mod.params['zbp'].value=1.75
#mod.params['zbp'].pmin=1.40#1.50
#mod.params['zbp'].pmax=2.10#2.00

#mod.params['zbq'].value=2.0
#mod.params['zbq'].pmin=1.50
#mod.params['zbq'].pmax=2.50
#mod.params['zbq'].fixed=0

#MB-change
mod.params['zbq'].value=1.75
mod.params['zbq'].pmin=1.40#1.50
mod.params['zbq'].pmax=2.10#2.00

mod.filename=mfile
mod.run(obsfile,outfile=outfile)
