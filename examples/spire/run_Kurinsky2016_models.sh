#!/usr/bin/env python
import os
import sys
import datetime
import time

codedir='/usr/local/surveysim/'
#try to import installed python module
try:
    from SurveySim.ModelFile import *
except:
    os.path.append(codedir+'python/')

basedir=codedir+'examples/spire/'
if(len(sys.argv) < 4):
    print "Calling Sequence: "+sys.argv[0]+" field(0=COSMOS,2=COSMOS+MIPS) model(0=onlySFG,1=agn,2=composites,3=cold, 4=SFG_cold) lfForm(0=MS,1=DPL,2=S)"
    quit()
else:
    field=int(sys.argv[1])
    model=int(sys.argv[2])
    lfForm=int(sys.argv[3])

#initialize
mod=ModelFile()
mod.axis1="ColorF1F2"
mod.axis2="Flux1"

convergence=0.95
#run settings; should be same regardless of model
mod.annealing['temp']=.03
mod.annealing['learningRate']=0.4
mod.convergence['CI']=convergence

mod.filters[0].setID("SPIRE_250")
mod.filters[1].setID("SPIRE_350")
mod.filters[2].setID("SPIRE_500")

#set parameters for field, and input file
if(field == 0):
    simname="spire_COSMOS"
    obsfile=basedir+"L2-COSMOS_xID250_DR2.fits"
    mod.survey['area']=4.78
    mod.filters[0].limit=8.0
    mod.filters[0].err=6.95
    mod.filters[0].compN=1.48
    mod.filters[0].compB=5.90
    mod.filters[0].compM=9.27
    mod.filters[1].limit=8.0
    mod.filters[1].err=6.63 
    mod.filters[2].limit=0.1
    mod.filters[2].err=6.63
elif(field == 2):
    simname="spire_mips_COSMOS"
    obsfile=basedir+"L2-COSMOS_xID24_DR3.fits"
    #load limits and filters from pre-made model file
    mod.load(basedir+"mips_default_model.fits")
    mod.convergence['CI']=convergence
    mod.filters[0].err=16.0 #median error in F24um
    mod.filters[1].err=2.0 #median total error in 250um (inst+conf)
    mod.filters[2].err=2.7 #median total error in 350um (inst+conf)
    mod.survey['area']=2.09
    mod.filters[1].limit=8.0
    mod.filters[2].limit=0.1

    mod.annealing['temp']=.002
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

mod.params['fa0'].value=0.25
mod.params['fa0'].pmin=0.10
mod.params['fa0'].pmax=0.50
mod.params['fa0'].fixed=0

mod.params['t1'].value=-0.1
mod.params['t1'].pmin=-1.0
mod.params['t1'].pmax=6.00
mod.params['t1'].fixed=0

mod.params['t2'].value=5.00
mod.params['t2'].pmin=-6.00
mod.params['t2'].pmax=6.00
mod.params['t2'].fixed=0

mod.params['zbt'].value=2.50
mod.params['zbt'].pmin=1.5
mod.params['zbt'].pmax=3.5
mod.params['zbt'].fixed=0


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

if(model == 0 and lfForm == 1):
    simname="A"
if(model == 0 and lfForm == 0):
    simname="B"
if(model == 1 and lfForm == 1):
    simname="C"
if(model == 1 and lfForm == 0):
    simname="D"
if(model == 2 and lfForm == 1):
    simname="E"
if(model == 2 and lfForm == 0):
    simname="F"

if(field == 0):
    simname=simname+'_spire'
if(field == 2):
    simname=simname+'_mips'

mfile=simname+"_model.fits"
outfile=simname+"_output.fits"

#parameters below should be the same regardless of model

mod.params['Alpha'].fixed=1
mod.params['Beta'].fixed=1

if(lfForm == 0):
    mod.params['Alpha'].value=1.15
    mod.params['Beta'].value=0.52
    mod.params['Phi0'].value=-2.348
    mod.params['L0'].value=10.10
    mod.params['Phi0'].pmin=-2.4
    mod.params['Phi0'].pmax=-2.2
    mod.params['Phi0'].fixed=0
    mod.params['L0'].pmin=10.0
    mod.params['L0'].pmax=10.2
    mod.params['L0'].fixed=0

if(lfForm == 1):
    mod.params['Alpha'].value=2.6
    mod.params['Beta'].value=0.60
    mod.params['Phi0'].value=-3.248
    mod.params['L0'].value=10.85    
    mod.params['Phi0'].pmin=-3.35
    mod.params['Phi0'].pmax=-3.15
    mod.params['Phi0'].fixed=0
    mod.params['L0'].pmin=10.75
    mod.params['L0'].pmax=10.95
    mod.params['L0'].fixed=0
    
mod.params['P'].value=-0.57
mod.params['P'].fixed=0
mod.params['P'].pmin=-6.00
mod.params['P'].pmax=6.00

mod.params['P2'].fixed=0
mod.params['P2'].value=-2.40
mod.params['P2'].pmin=-6.00
mod.params['P2'].pmax=6.00

mod.params['Q'].value=3.55
mod.params['Q'].fixed=0
mod.params['Q'].pmin=-6.00
mod.params['Q'].pmax=6.00

mod.params['Q2'].fixed=0
mod.params['Q2'].value=0.8
mod.params['Q2'].pmin=-6.00
mod.params['Q2'].pmax=6.00

mod.params['zbp'].fixed=0
mod.params['zbp'].value=1.10
mod.params['zbp'].pmin=0.50
mod.params['zbp'].pmax=3.5

mod.params['zbq'].fixed=0
mod.params['zbq'].value=1.85
mod.params['zbq'].pmin=0.50
mod.params['zbq'].pmax=3.5

mod.survey['AGNexp']=12.00
mod.settings['verbosity']=3
mod.filename=mfile
mod.update()
#mod.run(obsfile,outfile=outfile,templatefile=codedir+"templates/default_templates.fits")