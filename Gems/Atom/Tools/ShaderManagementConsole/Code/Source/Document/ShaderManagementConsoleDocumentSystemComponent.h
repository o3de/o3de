/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

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
