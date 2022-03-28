"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
import azlmbr.shader
import collections
from PySide2 import QtWidgets

PROJECT_SHADER_VARIANTS_FOLDER = "ShaderVariants"

def clean_existing_shadervariantlist_files(filePaths):
    for file in filePaths:
        if os.path.exists(file):
            os.remove(file)

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

    # prompt the user to save the file in the project folder or same folder as the shader 
    msgBox = QtWidgets.QMessageBox(
        QtWidgets.QMessageBox.Question,
        "Choose Save Location for .shadervariantlist File",
        "Save .shadervariantlist file in Project folder or in the same folder as shader file?"
    )

    projectButton = msgBox.addButton("Project Folder", QtWidgets.QMessageBox.AcceptRole)
    shaderButton = msgBox.addButton("Same Folder as Shader", QtWidgets.QMessageBox.AcceptRole)
    cancelButton = msgBox.addButton("Cancel", QtWidgets.QMessageBox.RejectRole)
    msgBox.exec()
    
    is_save_in_project_folder = False
    if msgBox.clickedButton() == projectButton:
        is_save_in_project_folder = True
    elif msgBox.clickedButton() == cancelButton:
        return

    # open the shader file as a document which will generate all of the shader variant list data 
    documentId = azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        azlmbr.bus.Broadcast,
        'OpenDocument',
        filename
    )

    if documentId.IsNull():
        print("The shader source data file could not be opened")
        return

    # get the shader variant list data object which is only needed for the shader file path
    shaderVariantList = azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
        azlmbr.bus.Event,
        'GetShaderVariantListSourceData',
        documentId
    )

    # generate the default save file path by replacing the extension of the open shader file
    pre, ext = os.path.splitext(filename)
    defaultShaderVariantListFilePath = f'{pre}.shadervariantlist'
    
    # clean previously generated shader variant list file so they don't clash.
    pre, ext = os.path.splitext(shaderVariantList.shaderFilePath)
    projectShaderVariantListFilePath = os.path.join(azlmbr.paths.projectroot, PROJECT_SHADER_VARIANTS_FOLDER, f'{pre}.shadervariantlist')
    
    clean_existing_shadervariantlist_files([
        projectShaderVariantListFilePath
    ])

    # Save the shader variant list file
    if is_save_in_project_folder:
        shaderVariantListFilePath = projectShaderVariantListFilePath
    else:
        shaderVariantListFilePath = defaultShaderVariantListFilePath

    shaderVariantListFilePath = shaderVariantListFilePath.replace("\\", "/")

    # Save the document in shader management console
    saveResult = azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        azlmbr.bus.Broadcast,
        'SaveDocumentAsChild',
        documentId,
        shaderVariantListFilePath
    )

    if saveResult:
        msgBox = QtWidgets.QMessageBox(
            QtWidgets.QMessageBox.Information,
            "Shader Variant List File Successfully Generated",
            f".shadervariantlist file was saved in:\n{shaderVariantListFilePath}",
            QtWidgets.QMessageBox.Ok
        )
        msgBox.exec()

    print("==== End shader variant script ============================================================")

if __name__ == "__main__":
    main()

