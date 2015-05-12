#!/usr/bin/env python

import os
import sys
import datetime
import time
sys.path.append("../Python/")
from OutputFile import *

if (len(sys.argv) < 2):
    print "Please provide model file name"
else:
    file=sys.argv[1]
    output=OutputFile(file)
    output.info()
    output.show()
    if(output.fit()):
        output.MCMC.showFit()
        output.MCMC.showChains()
    print ""
