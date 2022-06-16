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
        return AZStd::string::format(
            "@gemroot:MaterialCanvas@/Assets/MaterialCanvas/TestData/%s.materialcanvasnode.azasset", baseName.c_str());
    }

    void CreateTestNodes()
    {
        const AZ::Color defaultColor = AZ::Color::CreateOne();

        AtomToolsFramework::DynamicNodeConfig nodeConfig;
        nodeConfig.m_category = "Color Operations";
        nodeConfig.m_title = "Combine Color Channels";
        nodeConfig.m_inputSlots.emplace_back("inR", "R", "Red color channel", AZStd::any(defaultColor.GetR()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inG", "G", "Green color channel", AZStd::any(defaultColor.GetG()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inB", "B", "Blue color channel", AZStd::any(defaultColor.GetB()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inA", "A", "Alpha color channel", AZStd::any(defaultColor.GetA()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outColor", "Color", "Color", AZStd::any(""), AZStd::vector<AZStd::string>{ "Color" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig = {};
        nodeConfig.m_category = "Color Operations";
        nodeConfig.m_title = "Split Color";
        nodeConfig.m_outputSlots.emplace_back("outR", "R", "Red color channel", AZStd::any(defaultColor.GetR()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outG", "G", "Green color channel", AZStd::any(defaultColor.GetG()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outB", "B", "Blue color channel", AZStd::any(defaultColor.GetB()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outA", "A", "Alpha color channel", AZStd::any(defaultColor.GetA()), AZStd::vector<AZStd::string>{ "float" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inColor", "Color", "Color", AZStd::any(defaultColor), AZStd::vector<AZStd::string>{ "Color" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig = {};
        nodeConfig.m_category = "Color Operations";
        nodeConfig.m_title = "Add Color";
        nodeConfig.m_inputSlots.emplace_back("inColor1", "Color1", "Color1", AZStd::any(defaultColor), AZStd::vector<AZStd::string>{ "Color" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inColor2", "Color2", "Color2", AZStd::any(defaultColor), AZStd::vector<AZStd::string>{ "Color" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outColor", "Color", "Color", AZStd::any(defaultColor), AZStd::vector<AZStd::string>{ "Color" }, AtomToolsFramework::DynamicNodeSettingsMap());
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Subtract Color";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Multiply Color";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Divide Color";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));
    }
} // namespace MaterialCanvas
