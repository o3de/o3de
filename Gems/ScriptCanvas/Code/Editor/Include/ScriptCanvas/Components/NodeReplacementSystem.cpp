/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodeReplacementSystem.h"

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/string/string_view.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

namespace ScriptCanvasEditor
{
    static constexpr const char NodeReplacementRootPath[] = "/O3DE/NodeReplacement/ScriptCanvas";
    static constexpr const char NodeReplacementOldNodeFieldName[] = "OldNode";
    static constexpr const char NodeReplacementNewNodeFieldName[] = "NewNode";
    static constexpr const char NodeReplacementUuidFieldName[] = "Uuid";
    static constexpr const char NodeReplacementClassFieldName[] = "Class";
    static constexpr const char NodeReplacementMethodFieldName[] = "Method";

    NodeReplacementId NodeReplacementSystem::GenerateReplacementId(const AZ::Uuid& id, const AZStd::string& className, const AZStd::string& methodName)
    {
        NodeReplacementId result = id.ToFixedString().c_str();
        if (!className.empty() && !methodName.empty())
        {
            result += AZStd::string::format("_%s_%s", className.c_str(), methodName.c_str());
        }
        else if (!methodName.empty())
        {
            result += AZStd::string::format("_%s", methodName.c_str());
        }
        return result;
    }

    NodeReplacementId NodeReplacementSystem::GenerateReplacementId(ScriptCanvas::Node* node)
    {
        if (node)
        {
            if (auto* methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(node))
            {
                return GenerateReplacementId(methodNode->RTTI_GetType(),
                    methodNode->GetRawMethodClassName().empty() ? "" : methodNode->GetMethodClassName(),
                    methodNode->GetName());
            }
            // extend further here to support all types of node
        }
        return "";
    }

    ScriptCanvas::NodeReplacementConfiguration NodeReplacementSystem::GetNodeReplacementConfiguration(
        const NodeReplacementId& replacementId) const
    {
        auto foundIter = m_replacementMetadata.find(replacementId);
        if (foundIter != m_replacementMetadata.end())
        {
            return foundIter->second;
        }
        return {};
    }

    void NodeReplacementSystem::LoadReplacementMetadata()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ_Assert(settingsRegistry, "Global Settings registry must be available to retrieve replacement metadata.");

        auto RetrieveNodeReplacementEntries = [this, &settingsRegistry](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::unordered_map<AZStd::string, ScriptCanvas::NodeReplacementConfiguration> tempReplacementMap;

            auto RetrieveNodeReplacementPair = [&settingsRegistry, &tempReplacementMap](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                if (visitArgs.m_fieldName == NodeReplacementOldNodeFieldName || visitArgs.m_fieldName == NodeReplacementNewNodeFieldName)
                {
                    ScriptCanvas::NodeReplacementConfiguration replacementConfig;
                    AZStd::string uuid;
                    if (settingsRegistry->Get(uuid, AZStd::string::format("%s/%s", visitArgs.m_jsonKeyPath.data(), NodeReplacementUuidFieldName)))
                    {
                        replacementConfig.m_type = AZ::Uuid::CreateString(uuid.c_str());
                    }
                    else
                    {
                        // Uuid must be presented
                        return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                    }

                    settingsRegistry->Get(
                        replacementConfig.m_className, AZStd::string::format("%s/%s", visitArgs.m_jsonKeyPath.data(), NodeReplacementClassFieldName));
                    settingsRegistry->Get(
                        replacementConfig.m_methodName, AZStd::string::format("%s/%s", visitArgs.m_jsonKeyPath.data(), NodeReplacementMethodFieldName));

                    tempReplacementMap.emplace(visitArgs.m_fieldName, replacementConfig);
                }

                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            };

            AZ::SettingsRegistryVisitorUtils::VisitObject(*settingsRegistry, RetrieveNodeReplacementPair, visitArgs.m_jsonKeyPath.data());
            // Assume replacement metadata should be a pair
            if (tempReplacementMap.size() >= 2)
            {
                ScriptCanvas::NodeReplacementConfiguration oldNode = tempReplacementMap[NodeReplacementOldNodeFieldName];
                NodeReplacementId replacementId = GenerateReplacementId(oldNode.m_type, oldNode.m_className, oldNode.m_methodName);
                ScriptCanvas::NodeReplacementConfiguration newNode = tempReplacementMap[NodeReplacementNewNodeFieldName];
                m_replacementMetadata.emplace(replacementId, newNode);
            }
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitArray(*settingsRegistry, RetrieveNodeReplacementEntries, NodeReplacementRootPath);
        NodeReplacementRequestBus::Handler::BusConnect();
    }

    void NodeReplacementSystem::UnloadReplacementMetadata()
    {
        NodeReplacementRequestBus::Handler::BusDisconnect();
        m_replacementMetadata.clear();
    }
}
