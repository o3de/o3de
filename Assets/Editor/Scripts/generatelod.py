#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
'''
Generates a 50% lod for the selected model

@argument name="Lod Percentage", type="string", default="50.0f"
'''

import sys
import time

percentage = float(sys.argv[1])

selectedcgf = lodtools.getselected()
selectedmaterial = lodtools.getselectedmaterial()

loadedmodel = lodtools.loadcgf(selectedcgf)
loadedmaterial = lodtools.loadmaterial(selectedmaterial)

if loadedmodel == True and loadedmaterial == True:
    lodtools.generatelodchain()
    finished = 0.0
    while finished >= 0.0:
        finished = lodtools.generatetick()
        print 'Lod Chain Generation progress: ' + str(finished)
        time.sleep(1)
        if finished == 1.0:
            break
    
    lodtools.createlod(1,percentage)
    lodtools.generatematerials(1,512,512)
    lodtools.savetextures(1)
    lodtools.savesettings()
    lodtools.reloadmodel()

