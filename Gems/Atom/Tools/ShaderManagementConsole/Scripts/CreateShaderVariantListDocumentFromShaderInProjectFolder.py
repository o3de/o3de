"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus
import azlmbr.paths
import GenerateShaderVariantListUtil

def main():
    print("==== Begin shader variant script ==========================================================")

    if len(sys.argv) != 2:
        print("The script requires a .shader file as input argument")
        return

    inputPath = sys.argv[1]
    rootPath, extension = os.path.splitext(inputPath)
    if extension != ".shader":
        print("The input argument for the script is not a valid .shader file")
        return

    projectShaderVariantListFolder = os.path.join(azlmbr.paths.projectroot, "ShaderVariants")
    outputPath = rootPath + ".shadervariantlist"
    outputPath = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(azlmbr.bus.Broadcast, 'GenerateRelativeSourcePath', outputPath)
    outputPath = os.path.join(projectShaderVariantListFolder, outputPath)

    # Create shader variant list document
    documentId = azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        azlmbr.bus.Broadcast,
        'CreateDocumentFromTypeName',
        'Shader Variant List'
    )

    # Update shader variant list
    azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
        azlmbr.bus.Event,
        'SetShaderVariantListSourceData',
        documentId,
        GenerateShaderVariantListUtil.create_shadervariantlist_for_shader(inputPath)
    )

    # Save the shader variant list
    documentId = azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        azlmbr.bus.Broadcast,
        'SaveDocumentAsCopy',
        documentId,
        outputPath
    )

    print("==== End shader variant script ============================================================")

if __name__ == "__main__":
    main()
