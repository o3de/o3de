"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import re
import GenerateShaderVariantListUtil

def main():
    print("==== Begin processing shader ==========================================================")
    # Search all .shader in Gems folder
    gemsdir = azlmbr.paths.engroot + '/Gems'
    for subdir, dirs, files in os.walk(gemsdir):
        for file in files:
            filename = os.path.join(subdir, file)
            if re.search('^.*\.shader$', filename):
                shaderVariantList, defaultShaderVariantListFilePath = GenerateShaderVariantListUtil.create_shadervariantlist_for_shader(filename)
                if shaderVariantList.shaderVariants:
                    azlmbr.shader.SaveShaderVariantListSourceData(defaultShaderVariantListFilePath, shaderVariantList)

    print("==== Finish processing shader ==========================================================")
if __name__ == "__main__":
    main()