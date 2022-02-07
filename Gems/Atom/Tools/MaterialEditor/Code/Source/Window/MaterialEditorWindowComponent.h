/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>

#include <AtomToolsFramework/Window/AtomToolsMainWindowFactoryRequestBus.h>
#include <Source/Window/MaterialEditorBrowserInteractions.h>
#include <Source/Window/MaterialEditorWindow.h>

namespace MaterialEditor
{
    //! MaterialEditorWindowComponent is the entry point for the Material Editor gem user interface, and is mainly
    //! used for initialization and registration of other classes, including MaterialEditorWindow.
    class MaterialEditorWindowComponent
        : public AZ::Component
        , private AzToolsFramework::EditorWindowRequestBus::Handler
        , private AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MaterialEditorWindowComponent, "{03976F19-3C74-49FE-A15F-7D3CADBA616C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        ////////////////////////////////////////////////////////////////////////
        // AtomToolsMainWindowFactoryRequestBus::Handler overrides...
        void CreateMainWindow() override;
        void DestroyMainWindow() override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<MaterialEditorWindow> m_window;
        AZStd::unique_ptr<MaterialEditorBrowserInteractions> m_materialEditorBrowserInteractions;
    };
}
