/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

#include <Atom/Document/MaterialDocumentNotificationBus.h>
#include <Atom/Document/MaterialDocumentSettings.h>
#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Document/MaterialDocument.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QString>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    //! MaterialDocumentSystemComponent is the central component of the Material Editor Core gem
    class MaterialDocumentSystemComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private MaterialDocumentNotificationBus::Handler
        , private MaterialDocumentSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MaterialDocumentSystemComponent, "{58ABE0AE-2710-41E2-ADFD-E2D67407427D}");

        MaterialDocumentSystemComponent();
        ~MaterialDocumentSystemComponent() = default;
        MaterialDocumentSystemComponent(const MaterialDocumentSystemComponent&) = delete;
        MaterialDocumentSystemComponent& operator=(const MaterialDocumentSystemComponent&) = delete;

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
        // MaterialDocumentNotificationBus::Handler overrides...
        void OnDocumentDependencyModified(const AZ::Uuid& documentId) override;
        void OnDocumentExternallyModified(const AZ::Uuid& documentId) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // MaterialDocumentSystemRequestBus::Handler overrides...
        AZ::Uuid CreateDocument() override;
        bool DestroyDocument(const AZ::Uuid& documentId) override;
        AZ::Uuid OpenDocument(AZStd::string_view sourcePath) override;
        AZ::Uuid CreateDocumentFromFile(AZStd::string_view sourcePath, AZStd::string_view targetPath) override;
        bool CloseDocument(const AZ::Uuid& documentId) override;
        bool CloseAllDocuments() override;
        bool CloseAllDocumentsExcept(const AZ::Uuid& documentId) override;
        bool SaveDocument(const AZ::Uuid& documentId) override;
        bool SaveDocumentAsCopy(const AZ::Uuid& documentId, AZStd::string_view targetPath) override;
        bool SaveDocumentAsChild(const AZ::Uuid& documentId, AZStd::string_view targetPath) override;
        bool SaveAllDocuments() override;
        ////////////////////////////////////////////////////////////////////////

        AZ::Uuid OpenDocumentImpl(AZStd::string_view sourcePath, bool checkIfAlreadyOpen);

        AZStd::intrusive_ptr<MaterialDocumentSettings> m_settings;
        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<MaterialDocument>> m_documentMap;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsToRebuild;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsToReopen;
        const size_t m_maxMessageBoxLineCount = 15;
    };
} // namespace MaterialEditor
