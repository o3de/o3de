"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import re
import GenerateShaderVariantListUtil

def search_and_save_shadervariant(searchPath):
    for subdir, dirs, files in os.walk(searchPath):
        for file in files:
            filename = os.path.join(subdir, file)
            if re.search('^.*\.shader$', filename):
                shaderVariantList, defaultShaderVariantListFilePath = GenerateShaderVariantListUtil.create_shadervariantlist_for_shader(filename)
                if shaderVariantList.shaderVariants:
                    azlmbr.shader.SaveShaderVariantListSourceData(defaultShaderVariantListFilePath, shaderVariantList)


def main():
    print("==== Begin processing shader ==========================================================")
    # Search all .shader in Engine Gems folder
    search_and_save_shadervariant(azlmbr.paths.engroot + '/Gems')

    # Search in project folder
    search_and_save_shadervariant(azlmbr.paths.projectroot + '/Shaders')

    print("==== Finish processing shader ==========================================================")
if __name__ == "__main__":
    main()