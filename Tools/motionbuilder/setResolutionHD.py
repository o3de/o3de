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

lSystem = FBSystem()

myCamera = FBCamera('charCamera')
myCamera.Parent = FBFindModelByName('Bip01')
myCamera.Show = True
myCamera.Selected = True

FBApplication().SwitchViewerCamera(myCamera)

myCamera.FrameSizeMode = FBCameraFrameSizeMode.kFBFrameSizeFixedResolution

myCamera.ResolutionHeight = 1080
myCamera.ResolutionWidth = 1920
myCamera.PixelAspectRatio = 1
myCamera.UseFrameColor = True