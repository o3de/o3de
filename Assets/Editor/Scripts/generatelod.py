#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

