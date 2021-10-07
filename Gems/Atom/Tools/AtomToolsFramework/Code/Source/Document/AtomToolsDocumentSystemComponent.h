/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemSettings.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QString>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    //! AtomToolsDocumentSystemComponent is the central component of the Material Editor Core gem
    class AtomToolsDocumentSystemComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AtomToolsDocumentNotificationBus::Handler
        , private AtomToolsDocumentSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(AtomToolsDocumentSystemComponent, "{343A3383-6A59-4343-851B-BF84FC6CB18E}");

        AtomToolsDocumentSystemComponent();
        ~AtomToolsDocumentSystemComponent() = default;
        AtomToolsDocumentSystemComponent(const AtomToolsDocumentSystemComponent&) = delete;
        AtomToolsDocumentSystemComponent& operator=(const AtomToolsDocumentSystemComponent&) = delete;

        static void Reflect(AZ::ReflectContext* context);

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
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentDependencyModified(const AZ::Uuid& documentId) override;
        void OnDocumentExternallyModified(const AZ::Uuid& documentId) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AtomToolsDocumentSystemRequestBus::Handler overrides...
        void RegisterDocumentType(AZStd::function<AtomToolsDocument*()> documentCreator) override;
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

        AZStd::intrusive_ptr<AtomToolsDocumentSystemSettings> m_settings;
        AZStd::function<AtomToolsDocument*()> m_documentCreator;
        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<AtomToolsDocument>> m_documentMap;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsToRebuild;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsToReopen;
        const size_t m_maxMessageBoxLineCount = 15;
    };
} // namespace AtomToolsFramework
