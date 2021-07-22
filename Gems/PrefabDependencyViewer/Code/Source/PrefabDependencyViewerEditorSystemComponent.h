/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
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
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

#include <PrefabDependencyTreeGenerator.h>

namespace PrefabDependencyViewer
{
    using InstanceEntityMapperInterface  = AzToolsFramework::Prefab::InstanceEntityMapperInterface;
    using PrefabSystemComponentInterface = AzToolsFramework::Prefab::PrefabSystemComponentInterface;
    using InstanceOptionalReference      = AzToolsFramework::Prefab::InstanceOptionalReference;
    using Instance                       = AzToolsFramework::Prefab::Instance;
    using TemplateId                     = AzToolsFramework::Prefab::TemplateId;
    using PrefabDom                      = AzToolsFramework::Prefab::PrefabDom;
    using Outcome                        = AZ::Outcome<PrefabDependencyTree, const char*>;

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

        static Outcome GenerateTreeAndSetRoot(TemplateId tid)
        {
            return PrefabDependencyTree::GenerateTreeAndSetRoot(tid,
                                            s_prefabSystemComponentInterface);

        }

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
    };
} // namespace PrefabDependencyViewer
