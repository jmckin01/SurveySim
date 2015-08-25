#!/usr/bin/env python

import os
import sys
import datetime
import time
from ModelFile import *

mfile="/Volumes/Pepona/Research/wise_populations/alma_test/models/model_dpl_a.fits"
templatefile="../templates/default_templates.fits"
outfile="/Volumes/Pepona/Research/wise_populations/alma_test/outputs/output_dpl_a_002.fits"

mod=ModelFile()

mod.axis1="ColorF2F3"
mod.axis2="Flux1"

mod.survey['area']=1.0 #deg2

mod.filters[0].setID('ALMA_band7')        
mod.filters[0].limit=3.5               # S/N limit
mod.filters[0].err=1.229               # rms1
mod.filters[0].serr=0.00
mod.filters[0].unit='mJy'

#just place-holders we're only interested in ALMA-band7 here.
mod.filters[1].setID("SPIRE_250")
mod.filters[1].limit=0
mod.filters[1].err=0
mod.filters[1].serr=0.00
mod.filters[1].unit=='mJy'

mod.filters[2].setID("SPIRE_350") 
mod.filters[2].limit=0                 
mod.filters[2].err=0
mod.filters[2].serr=0.00
mod.filters[2].unit='mJy'

print mod.filters[0]

#mod.setLF("ModifiedSchecter")
mod.setLF("DoublePowerLaw")

mod.params['Phi0'].value=-3.43
mod.params['Phi0'].fixed=1

mod.params['L0'].value=11.02
mod.params['L0'].fixed=1

mod.params['Alpha'].value=2.04
mod.params['Alpha'].fixed=1

mod.params['Beta'].value=0.56
mod.params['Beta'].fixed=1

mod.params['zbp'].value=2.54
mod.params['zbq'].value=1.64
mod.params['zbp'].fixed=1
mod.params['zbq'].fixed=1

mod.params['P'].value=-3.44
mod.params['P'].fixed=1

mod.params['Q'].value=4.46
mod.params['Q'].fixed=1

mod.params['P2'].value=2.88  #was -2.88
mod.params['P2'].fixed=1

mod.params['Q2'].value=1.49
mod.params['Q2'].fixed=1

mod.params['cexp'].value=0
mod.params['cexp'].fixed=1

mod.settings['verbosity']=1
mod.annealing['temp']=.02
mod.annealing['learningRate']=0.1

mod.convergence['CI']=0.90

mod.redshift['min']=0.05 

mod.write(mfile)
#mod.info()

os.system('SurveySim '+mfile+' '+templatefile+' -o '+outfile)
