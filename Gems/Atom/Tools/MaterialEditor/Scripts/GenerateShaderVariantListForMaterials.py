"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import sys
import os
import azlmbr.asset as asset
import azlmbr.atom
import azlmbr.bus
import azlmbr.math as math
import azlmbr.name
import azlmbr.paths
import azlmbr.shadermanagementconsole


def main():
    print("==== Begin shader variant script ==========================================================")

    if len(sys.argv) != 2:
        print("The script requires a .shader file as input argument")
        return

    filename = sys.argv[1]
    suffix, extension = filename.split(".", 1)

    if extension != "shader":
        print("The input argument for the script is not a valid .shader file")
        return
    
    azlmbr.shadermanagementconsole.ShaderManagementConsoleWindowRequestBus(azlmbr.bus.Broadcast, 'GenerateShaderVariantListForShaderMaterials', filename)

    print("==== End shader variant script ============================================================")

if __name__ == "__main__":
    main()

