"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import GenerateShaderVariantListUtil

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

    shaderVariantList = GenerateShaderVariantListUtil.create_shadervariantlist_for_shader(filename)

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
        shaderVariantList
    )
    
    print("==== End shader variant script ============================================================")

if __name__ == "__main__":
    main()

