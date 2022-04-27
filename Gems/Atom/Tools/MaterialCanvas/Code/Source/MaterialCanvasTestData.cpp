/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace MaterialCanvas
{
    AZStd::string GetTestDataPathForNodeConfig(const AtomToolsFramework::DynamicNodeConfig& nodeConfig)
    {
        AZStd::string baseName = AZ::RPI::AssetUtils::SanitizeFileName(nodeConfig.m_title);
        AZStd::to_lower(baseName.begin(), baseName.end());
        auto resolvedPath = AZ::IO::FileIOBase::GetInstance()->ResolvePath(AZ::IO::PathView{ AZStd::string::format(
            "@gemroot:MaterialCanvas@/Assets/MaterialCanvas/TestData/%s.materialcanvasnode.azasset", baseName.c_str()) });
        return resolvedPath ? resolvedPath->Native().c_str() : AZStd::string();
    }

    void CreateTestNodes()
    {
        AtomToolsFramework::DynamicNodeConfig nodeConfig;
        nodeConfig.m_category = "Color Operations";
        nodeConfig.m_title = "Combine Color Channels";
        nodeConfig.m_inputSlots.emplace_back("Data", "inR", "R", "Red color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_inputSlots.emplace_back("Data", "inG", "G", "Green color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_inputSlots.emplace_back("Data", "inB", "B", "Blue color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_inputSlots.emplace_back("Data", "inA", "A", "Alpha color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_outputSlots.emplace_back("Data", "outColor", "Color", "Color", AZStd::vector<AZStd::string>{ "Color" }, "");
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig = {};
        nodeConfig.m_category = "Color Operations";
        nodeConfig.m_title = "Split Color";
        nodeConfig.m_outputSlots.emplace_back("Data", "outR", "R", "Red color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_outputSlots.emplace_back("Data", "outG", "G", "Green color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_outputSlots.emplace_back("Data", "outB", "B", "Blue color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_outputSlots.emplace_back("Data", "outA", "A", "Alpha color channel", AZStd::vector<AZStd::string>{ "float" }, "");
        nodeConfig.m_inputSlots.emplace_back("Data", "inColor", "Color", "Color", AZStd::vector<AZStd::string>{ "Color" }, "");
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig = {};
        nodeConfig.m_category = "Color Operations";
        nodeConfig.m_title = "Add Color";
        nodeConfig.m_inputSlots.emplace_back("Data", "inColor1", "Color1", "Color1", AZStd::vector<AZStd::string>{ "Color" }, "");
        nodeConfig.m_inputSlots.emplace_back("Data", "inColor2", "Color2", "Color2", AZStd::vector<AZStd::string>{ "Color" }, "");
        nodeConfig.m_outputSlots.emplace_back("Data", "outColor", "Color", "Color", AZStd::vector<AZStd::string>{ "Color" }, "");
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Subtract Color";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Multiply Color";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Divide Color";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));
    }
} // namespace MaterialCanvas
