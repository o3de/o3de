#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/Data/ShaderVariantStatisticData.cpp
    Source/Data/ShaderVariantStatisticData.h
    
    Source/Document/ShaderManagementConsoleDocumentRequestBus.h
    Source/Document/ShaderManagementConsoleDocument.cpp
    Source/Document/ShaderManagementConsoleDocument.h

    Source/Window/ShaderManagementConsoleTableView.h
    Source/Window/ShaderManagementConsoleTableView.cpp
    Source/Window/ShaderManagementConsoleStatisticView.h
    Source/Window/ShaderManagementConsoleStatisticView.cpp
    Source/Window/ShaderManagementConsoleWindow.h
    Source/Window/ShaderManagementConsoleWindow.cpp
    Source/Window/ShaderManagementConsole.qrc

    Source/main.cpp
    Source/ShaderManagementConsoleApplication.cpp
    Source/ShaderManagementConsoleApplication.h
    Source/ShaderManagementConsoleRequestBus.h
    ../Scripts/CreateShaderVariantListDocumentFromShader.py
    ../Scripts/ExpandOptionsFullCombinatorials.py
    ../Scripts/GenerateShaderVariantListForAllShaders.py
)
