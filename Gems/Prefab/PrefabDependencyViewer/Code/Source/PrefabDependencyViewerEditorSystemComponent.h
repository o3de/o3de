/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>

#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>

#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

#include <DirectedGraph.h>

#include <PrefabDependencyTree.h>

namespace PrefabDependencyViewer
{
    using InstanceEntityMapperInterface  = AzToolsFramework::Prefab::InstanceEntityMapperInterface;
    using PrefabSystemComponentInterface = AzToolsFramework::Prefab::PrefabSystemComponentInterface;
    using PrefabPublicInterface          = AzToolsFramework::Prefab::PrefabPublicInterface;
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

        //////////////////////////////////// EditorContextMenuBus overrides ///////////////////////////
        int GetMenuPosition() const override;
        AZStd::string GetMenuIdentifier() const override;

        /**
         * Adds the view Dependencies option when an entity gets
         * clicked on in the Editor. It also adds a handler for it's trigger event.
         * The handler name is ContextMenu_DisplayAssetDependencies.
         */
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;

        /////////////////////////////////// EditorEventsBus overrides //////////////////////////////////
        void NotifyRegisterViews() override;

        /**
         * Generate the Prefab hierarchy. Set the root of the hierarchy to be the
         * Prefab Template with the id = param tid. Returns the hierarchy of given Prefab
         * Template if no errors occurred. Otherwise, it returns the error message.
         * 
         * @param tid Id of the Prefab Template whose hierarchy we want to generate.
         * @return    Outcome which contains the graph if no errors occurred during
         *            the generation of the graph. Otherwise, it contains the description
         *            of the error that occurred.
         */
        Outcome GenerateTreeAndSetRoot(TemplateId tid);

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        /**
         * @param tid Id of the Template from which the Prefab Instance was created.
         * It's the handler for the trigger event on View Dependencies menu option
         * on the container entity of a Prefab Instance. Opens a New Graph Canvas
         * Window and displays the prefab dependencies using it's Template.
         */ 
        void ContextMenu_DisplayAssetDependencies(const TemplateId& tid);

        InstanceEntityMapperInterface* m_prefabEntityMapperInterface     = nullptr;
        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        PrefabPublicInterface* m_prefabPublicInterface                   = nullptr;

        static constexpr inline const char* s_prefabViewerTitle = "Prefab Dependencies Viewer";
    };
} // namespace PrefabDependencyViewer
