"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import os
import re
import sys

import azlmbr.bus
import azlmbr.paths
import collections
import GenerateShaderVariantListUtil

from PySide2 import QtWidgets

PROJECT_SHADER_VARIANTS_FOLDER = "ShaderVariants"

def main():
    print("==== Begin generating shader variant lists for all shaders ==========")

    paths = azlmbr.atomtools.util.GetPathsInSourceFoldersMatchingExtension('shader')

    progressDialog = QtWidgets.QProgressDialog("Batch generating shader variant lists...", "Cancel", 0, len(paths))
    progressDialog.setMaximumWidth(400)
    progressDialog.setMaximumHeight(100)
    progressDialog.setModal(True)
    progressDialog.setWindowTitle("Batch generating shader variant lists")

    for i, path in enumerate(paths):
        shaderVariantList = GenerateShaderVariantListUtil.create_shadervariantlist_for_shader(path)
        if len(shaderVariantList.shaderVariants) == 0:
            continue

        relativePath = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(azlmbr.bus.Broadcast, 'GenerateRelativeSourcePath', path)

        pre, ext = os.path.splitext(relativePath)
        savePath = os.path.join(azlmbr.paths.projectroot, PROJECT_SHADER_VARIANTS_FOLDER, f'{pre}.shadervariantlist')

        # clean previously generated shader variant list file so they don't clash.
        if os.path.exists(savePath):
            os.remove(savePath)
            
        azlmbr.shader.SaveShaderVariantListSourceData(savePath, shaderVariantList)

        progressDialog.setValue(i)
        if progressDialog.wasCanceled():
            return
    progressDialog.close()

    print("==== Finish generating shader variant lists for all shaders ==========")

if __name__ == "__main__":
    main()