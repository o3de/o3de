/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/stack.h>
#include <AzCore/std/utils.h>
#include <AzCore/Outcome/Outcome.h>

namespace PrefabDependencyViewer
{
    class PrefabDependencyTree;

    using TemplateId                     = AzToolsFramework::Prefab::TemplateId;
    using PrefabDom                      = AzToolsFramework::Prefab::PrefabDom;
    using PrefabSystemComponentInterface = AzToolsFramework::Prefab::PrefabSystemComponentInterface;
    using Outcome                        = AZ::Outcome<PrefabDependencyTree, const char*>;

    class PrefabDependencyTree : public Utils::DirectedGraph
    {
    public:
        static Outcome GenerateTreeAndSetRoot(TemplateId tid,
            PrefabSystemComponentInterface* s_prefabSystemComponentInterface)
        {
            PrefabDependencyTree graph;

            const TemplateId InvalidTemplateId = AzToolsFramework::Prefab::InvalidTemplateId;

            using pair = AZStd::pair<TemplateId, Utils::Node*>;
            AZStd::stack<pair> stack;
            stack.push(AZStd::make_pair(tid, nullptr));

            while (!stack.empty())
            {
                // Get the current TemplateId we are looking at and it's parent.
                pair tidAndParent = stack.top();
                stack.pop();

                TemplateId templateId = tidAndParent.first;

                if (InvalidTemplateId == templateId)
                {
                    return AZ::Failure("PrefabDependencyTreeGenerator - Invalid TemplateId found");
                }

                Utils::Node* parent = tidAndParent.second;

                // Get the JSON for the current Template we are looking at.
                PrefabDom& prefabDom = s_prefabSystemComponentInterface->FindTemplateDom(templateId);

                // Get the source file of the current Template
                auto sourceIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
                if (sourceIterator == prefabDom.MemberEnd() || !sourceIterator->value.IsString())
                {
                    return AZ::Failure("PrefabDependencyTree - Source Attribute missing or it's not a String");
                }

                auto&& source = sourceIterator->value;
                const char* sourceFileName = source.GetString();

                // Create a new node for the current Template and
                // Connect it to it's parent.
                Utils::Node* node = aznew Utils::Node(templateId, sourceFileName);
                graph.AddNode(node);
                graph.AddChild(parent, node);

                // Go through current Template's nested instances
                // and put their TemplateId and current Template node
                // as their parent on the stack.
                auto instancesIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::InstancesName);

                if (instancesIterator != prefabDom.MemberEnd())
                {
                    auto&& instances = instancesIterator->value;

                    if (instances.IsObject())
                    {
                        for (auto&& instance : instances.GetObject())
                        {
                            // Get the source file of the current Template
                            sourceIterator = instance.value.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
                            sourceFileName = "";
                            if (sourceIterator != instance.value.MemberEnd())
                            {
                                source = sourceIterator->value;
                                sourceFileName = source.IsString() ? source.GetString() : "";
                            }

                            // Checks for InvalidTemplateId when we pop the element off of the stack above.
                            TemplateId childtid = s_prefabSystemComponentInterface->GetTemplateIdFromFilePath(sourceFileName);
                            stack.push(AZStd::make_pair(childtid, node));
                        }
                    }
                }
            }

            return AZ::Success(graph);
        }
    };
}
