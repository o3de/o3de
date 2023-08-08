"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import re
import json
import GenerateShaderVariantListUtil

def main():
    print("==== Begin generate shader variant list for all built material assets ==========")
    
    assetIds = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(
        azlmbr.bus.Broadcast, 
        'GetAllMaterialAssetIds'
    )
    GenerateShaderVariantListUtil.save_shadervariantlists_for_materials(assetIds)

    print("==== Finish generate shader variant list for all built material assets ==========")

if __name__ == "__main__":
    main()