/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>

#include <ImageProcessing/ImageProcessingBus.h>
#include <ImageProcessing/ImageProcessingEditorBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

namespace ImageProcessing
{
    class ImageProcessingSystemComponent
        : public AZ::Component
        , protected ImageProcessingRequestBus::Handler
        , protected ImageProcessingEditor::ImageProcessingEditorRequestBus::Handler
        , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , protected AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequestsBus::Handler
    {
    public:
        AZ_COMPONENT(ImageProcessingSystemComponent, "{13B1EB88-316F-4D44-B59C-886F023A5A58}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ImageProcessingEditorRequestBus interface implementation
        void OpenSourceTextureFile(const AZ::Uuid& textureSourceID) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ImageProcessingRequestBus interface implementation
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
        void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequestsBus::Handler
        bool GetProductTexturePreview(const char* fullProductFileName, QImage& previewImage, AZStd::string& productInfo, AZStd::string& productAlphaInfo) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        bool HandlesSource(AZStd::string_view fileName) const;

        bool LoadTextureSettings();

        bool m_textureSettingsLoaded = false;
    };
} // namespace ImageProcessing
