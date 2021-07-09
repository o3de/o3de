/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

#include <AzCore/Math/Vector2.h>
#include <Utils.h>
#include <AzToolsFramework\Prefab\PrefabDomUtils.h>

namespace PrefabDependencyViewer
{
    using InstanceEntityMapperInterface  = AzToolsFramework::Prefab::InstanceEntityMapperInterface;
    using PrefabSystemComponentInterface = AzToolsFramework::Prefab::PrefabSystemComponentInterface;
    using InstanceOptionalReference      = AzToolsFramework::Prefab::InstanceOptionalReference;
    using Instance                       = AzToolsFramework::Prefab::Instance;
    using TemplateId                     = AzToolsFramework::Prefab::TemplateId;
    using PrefabDom                      = AzToolsFramework::Prefab::PrefabDom;

    /// System component for PrefabDependencyViewer editor
    class PrefabDependencyViewerEditorSystemComponent
        : public AZ::Component
        , public AzToolsFramework::EditorContextMenuBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(PrefabDependencyViewerEditorSystemComponent, "{1eb2c3bf-ef82-4bb4-82a0-4b6bd2d9895c}");
        static void Reflect(AZ::ReflectContext* context);

        PrefabDependencyViewerEditorSystemComponent();

        // EditorContextMenuBus...
        int GetMenuPosition() const override;
        AZStd::string GetMenuIdentifier() const override;
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;

        // Opening a View Pane
        void NotifyRegisterViews() override;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        static void ContextMenu_DisplayAssetDependencies(const TemplateId& tid);

        static InstanceEntityMapperInterface* s_prefabEntityMapperInterface;
        static PrefabSystemComponentInterface* s_prefabSystemComponentInterface;

        static const char* s_prefabViewerTitle;
        static void GenerateTreeAndSetRoot(TemplateId tid, Utils::DirectedGraph& graph)
        {
            using pair = AZStd::pair<TemplateId, Utils::Node*>;
            AZStd::stack<pair> stack;
            stack.push(AZStd::make_pair(tid, nullptr));

            while (!stack.empty())
            {
                // Get the current TemplateId we are looking at
                // and it's parent.
                pair tidAndParent = stack.top();
                stack.pop();

                TemplateId templateId = tidAndParent.first;
                Utils::Node* parent = tidAndParent.second;

                // Get the JSON for the current Template we are looking at.
                PrefabDom& prefabDom = s_prefabSystemComponentInterface->FindTemplateDom(templateId);

                // Get the source file of the current Template
                auto sourceIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
                const char* sourceFileName = "";
                if (sourceIterator != prefabDom.MemberEnd())
                {
                    auto&& source = sourceIterator->value;
                    sourceFileName = source.IsString() ? source.GetString() : "";
                }

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
                                auto&& source = sourceIterator->value;
                                sourceFileName = source.IsString() ? source.GetString() : "";
                            }

                            TemplateId childtid = s_prefabSystemComponentInterface->GetTemplateIdFromFilePath(sourceFileName);
                            if (childtid != AzToolsFramework::Prefab::InvalidTemplateId)
                            {
                                stack.push(AZStd::make_pair(childtid, node));
                            }
                        }
                    }
                }
            }
        }
    };
} // namespace PrefabDependencyViewer
