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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>

#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Document/ShaderManagementConsoleDocument.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QString>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleDocumentSystemComponent is the central component of the Shader Management Console Core gem
    class ShaderManagementConsoleDocumentSystemComponent
        : public AZ::Component
        , private AzFramework::TmMsgBus::Handler
        , private ShaderManagementConsoleDocumentSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ShaderManagementConsoleDocumentSystemComponent, "{58ABE0AE-2710-41E2-ADFD-E2D67407427D}");

        ShaderManagementConsoleDocumentSystemComponent();
        ~ShaderManagementConsoleDocumentSystemComponent() = default;
        ShaderManagementConsoleDocumentSystemComponent(const ShaderManagementConsoleDocumentSystemComponent&) = delete;
        ShaderManagementConsoleDocumentSystemComponent& operator =(const ShaderManagementConsoleDocumentSystemComponent&) = delete;

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TmMsgBus::Handler overrides...
        void OnReceivedMsg(AzFramework::TmMsgPtr msg) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ShaderManagementConsoleDocumentSystemRequestBus::Handler overrides...
        AZ::Uuid CreateDocument() override;
        bool DestroyDocument(const AZ::Uuid& documentId) override;
        AZ::Uuid OpenDocument(AZStd::string_view path) override;
        bool CloseDocument(const AZ::Uuid& documentId) override;
        bool CloseAllDocuments() override;
        bool SaveDocument(const AZ::Uuid& documentId) override;
        bool SaveDocumentAsCopy(const AZ::Uuid& documentId) override;
        bool SaveAllDocuments() override;
        ////////////////////////////////////////////////////////////////////////

        AZ::Uuid OpenDocumentImpl(AZStd::string_view path, bool checkIfAlreadyOpen);

        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<ShaderManagementConsoleDocument>> m_documentMap;
    };
}
