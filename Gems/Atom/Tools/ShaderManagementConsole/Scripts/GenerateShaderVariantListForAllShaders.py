"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import re
import json
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
    search_and_save_shadervariant(azlmbr.paths.projectroot + '/Assets')

    # Search in externel gems
    # Read from external_subdirectories project.json
    projectPath = azlmbr.paths.projectroot + '/project.json'
    jsonFile = open(projectPath)
    data = json.load(jsonFile)

    for path in data['external_subdirectories']:
        search_and_save_shadervariant(azlmbr.paths.projectroot + '/' + path)

    jsonFile.close()

    print("==== Finish processing shader ==========================================================")
if __name__ == "__main__":
    main()