# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


import MaxPlus
import sys


def get_material_information():
    for mesh_object in MaxPlus.Core.GetRootNode().Children:
        print('Object---> {}'.format(mesh_object))


get_material_information()
