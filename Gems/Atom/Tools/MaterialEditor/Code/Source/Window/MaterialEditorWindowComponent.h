/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>

#include <Atom/Window/MaterialEditorWindowFactoryRequestBus.h>
#include <Source/Window/MaterialBrowserInteractions.h>
#include <Source/Window/MaterialEditorWindow.h>

namespace MaterialEditor
{
    //! MaterialEditorWindowComponent is the entry point for the Material Editor gem user interface, and is mainly
    //! used for initialization and registration of other classes, including MaterialEditorWindow.
    class MaterialEditorWindowComponent
        : public AZ::Component
        , private AzToolsFramework::EditorWindowRequestBus::Handler
        , private MaterialEditorWindowFactoryRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MaterialEditorWindowComponent, "{03976F19-3C74-49FE-A15F-7D3CADBA616C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        ////////////////////////////////////////////////////////////////////////
        // MaterialEditorWindowFactoryRequestBus::Handler overrides...
        void CreateMaterialEditorWindow() override;
        void DestroyMaterialEditorWindow() override;
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
        AZStd::unique_ptr<MaterialBrowserInteractions> m_materialBrowserInteractions;
    };
}
