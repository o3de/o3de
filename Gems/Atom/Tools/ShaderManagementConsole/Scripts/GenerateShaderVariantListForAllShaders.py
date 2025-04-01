"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import os
import re
import shutil
import sys

import azlmbr.bus
import azlmbr.paths
import collections
import GenerateShaderVariantListUtil

from PySide2 import QtWidgets

def main():
    msgBox = QtWidgets.QMessageBox()
    msgBox.setIcon(QtWidgets.QMessageBox.Information)
    msgBox.setWindowTitle("Generate shader variant lists for project")
    msgBox.setText("This process generates shader variant lists for your project and all active gems. Your project's ShaderVariants folder will be deleted and replaced with new shader variant lists. Make sure that the asset processor has finished processing all shader and material assets before proceeding because they will be used as part of this process. Continue?")
    msgBox.setStandardButtons(QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel)
    if msgBox.exec() != QtWidgets.QMessageBox.Ok:
        return

    print("==== Begin generating shader variant lists for all shaders ==========")

    projectShaderVariantListFolder = os.path.join(azlmbr.paths.projectroot, "ShaderVariants")
    projectShaderVariantListFolderTmp = os.path.join(azlmbr.paths.projectroot, "$tmp_ShaderVariants")

    # Remove the temporary shade of variant list folder in case it wasn't cleared last run.
    if os.path.exists(projectShaderVariantListFolderTmp):
        shutil.rmtree(projectShaderVariantListFolderTmp)

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
        savePath = os.path.join(projectShaderVariantListFolderTmp, f'{pre}.shadervariantlist')
           
        azlmbr.shader.SaveShaderVariantListSourceData(savePath, shaderVariantList)

        progressDialog.setValue(i)

        # processing events to update UI after progress bar changes
        QtWidgets.QApplication.processEvents()

        # Allowing the application to process idle events for one frame to update systems and garbage collect graphics resources
        azlmbr.atomtools.general.idle_wait_frames(1)
        if progressDialog.wasCanceled():
            return
    progressDialog.close()

    # Remove the final shader variant list folder and all of its contents
    if os.path.exists(projectShaderVariantListFolder):
        shutil.rmtree(projectShaderVariantListFolder)

    # Rename the temporary shader variant list folder to replace the final one
    if os.path.exists(projectShaderVariantListFolderTmp):
        os.rename(projectShaderVariantListFolderTmp, projectShaderVariantListFolder)

    print("==== Finish generating shader variant lists for all shaders ==========")

if __name__ == "__main__":
    main()