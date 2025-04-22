/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <Atom/ImageProcessing/ImageProcessingBus.h>
#include <Atom/ImageProcessing/ImageProcessingEditorBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerBus.h>

#include <Source/Previewer/ImagePreviewerFactory.h>

namespace ImageProcessingAtom
{
    class ImageProcessingSystemComponent
        : public AZ::Component
        , protected ImageProcessingRequestBus::Handler
        , protected ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler
        , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , protected AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ImageProcessingSystemComponent, "{AA1B93BF-8150-401A-8FF2-873B0C19299D}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AtomImageProcessingEditorRequestBus interface implementation
        void OpenSourceTextureFile(const AZ::Uuid& textureSourceID) override;
        ////////////////////////////////////////////////////////////////////////
        
        ////////////////////////////////////////////////////////////////////////
        // AtomImageProcessingRequestBus interface implementation
        IImageObjectPtr LoadImage(const AZStd::string& filePath) override;
        IImageObjectPtr LoadImagePreview(const AZStd::string& filePath) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationsBus::Handler
        void AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler
        const AzToolsFramework::AssetBrowser::PreviewerFactory* GetPreviewerFactory(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;
        ////////////////////////////////////////////////////////////////////////

    private:
        bool HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const;

        bool SaveStreamingImageAssetToDDS(const AZ::Data::AssetId& assetId, AZStd::string_view filePath);

        AZStd::unique_ptr<const ImagePreviewerFactory> m_previewerFactory;

        // Last saved dds file path
        QString m_lastSavedPath;
    };
} // namespace ImageProcessingAtom
