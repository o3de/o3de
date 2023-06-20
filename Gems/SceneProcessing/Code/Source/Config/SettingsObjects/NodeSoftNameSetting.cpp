/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <Config/SettingsObjects/NodeSoftNameSetting.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        NodeSoftNameSetting::NodeSoftNameSetting(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
            const char* virtualType, bool includeChildren)
            : SoftNameSetting(pattern, approach, virtualType)
            , m_includeChildren(includeChildren)
        {
        }

        void NodeSoftNameSetting::Reflect(ReflectContext* context)
        {
            SerializeContext* serialize = azrtti_cast<SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<NodeSoftNameSetting, SoftNameSetting>()
                    ->Version(1)
                    ->Field("includeChildren", &NodeSoftNameSetting::m_includeChildren);

                serialize->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<NodeSoftNameSetting>>>();

                EditContext* editContext = serialize->GetEditContext();
                if (editContext)
                {
                    editContext->Class<NodeSoftNameSetting>("Node name setting", "Applies the pattern to the name of the node.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Default, &NodeSoftNameSetting::m_includeChildren, 
                            "Include child nodes", "Whether or not the soft name only applies to the matching node or propagated to all its children as well.");
                }
            }
        }

        bool NodeSoftNameSetting::IsVirtualType(const SceneAPI::Containers::Scene& scene, SceneAPI::Containers::SceneGraph::NodeIndex node) const
        {
            const SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
            if (m_includeChildren)
            {
                auto upwardsView = SceneAPI::Containers::Views::MakeSceneGraphUpwardsView(graph, node, graph.GetNameStorage().begin(), true);
                for (const SceneAPI::Containers::SceneGraph::Name& name : upwardsView)
                {
                    if (MatchesPattern(name))
                    {
                        return true;
                    }
                }
                return false;
            }
            else
            {
                return MatchesPattern(graph.GetNodeName(node));
            }
        }

        const AZ::Uuid NodeSoftNameSetting::GetTypeId() const
        {
            return azrtti_typeid<NodeSoftNameSetting>();
        }

        bool NodeSoftNameSetting::MatchesPattern(const SceneAPI::Containers::SceneGraph::Name& name) const
        {
            switch (m_pattern.GetMatchApproach())
            {
            case SceneAPI::SceneCore::PatternMatcher::MatchApproach::PreFix:
                return m_pattern.MatchesPattern(name.GetName(), name.GetNameLength());
            case SceneAPI::SceneCore::PatternMatcher::MatchApproach::PostFix:
                return m_pattern.MatchesPattern(name.GetPath(), name.GetPathLength());
            case SceneAPI::SceneCore::PatternMatcher::MatchApproach::Regex:
                return m_pattern.MatchesPattern(name.GetPath(), name.GetPathLength());
            default:
                AZ_Assert(false, "Unknown option '%i' for pattern matcher.", m_pattern.GetMatchApproach());
                return false;
            }
        }

    } // namespace SceneProcessingConfig
} // namespace AZ
