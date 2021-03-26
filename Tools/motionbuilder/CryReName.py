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
from pyfbsdk import *

def UnselAll():
        selModels = FBModelList()
        FBGetSelectedModels (selModels, None, True)
        for model in selModels:
                model.Selected = False;
        del(selModels)
                

UnselAll()

Models = FBModelList()
Skeleton = list()
FBGetSelectedModels (Models, None, False)

for model in Models:
    if model.ClassName() == 'FBModelSkeleton':
        Skeleton.append(model)

Deter = FBFindModelByName("Bip01").Children[0].Name.split(":")        

for i in range(len(Skeleton)):  
        if Deter[1][5] == " ":
                SplName = Skeleton[i].Name.split(":")
                SplName[1] = SplName[1].replace(" ","_")
                Skeleton[i].Name = SplName[0] + ":" + SplName[1]
        else:
                SplName = Skeleton[i].Name.split(":")
                if SplName[1][0] == "_":
                        SplName[1] = SplName[1].replace("_"," ")
                        SplName[1] = SplName[1].replace(" ","_",1)
                        Skeleton[i].Name = SplName[0] + ":" + SplName[1]
                else:
                        SplName[1] = SplName[1].replace("_"," ")
                        Skeleton[i].Name = SplName[0] + ":" + SplName[1]
