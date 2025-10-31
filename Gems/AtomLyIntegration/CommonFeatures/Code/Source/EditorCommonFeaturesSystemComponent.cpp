/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <EditorCommonFeaturesSystemComponent.h>
#include <SharedPreview/SharedThumbnail.h>
#include <SkinnedMesh/SkinnedMeshDebugDisplay.h>

#include <IEditor.h>

namespace AZ
{
    namespace Render
    {
        EditorCommonFeaturesSystemComponent::EditorCommonFeaturesSystemComponent() = default;
        EditorCommonFeaturesSystemComponent::~EditorCommonFeaturesSystemComponent() = default;

        //! Main system component for the Atom Common Feature Gem's editor/tools module.
        void EditorCommonFeaturesSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorCommonFeaturesSystemComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Atom Level Default Asset Path", &EditorCommonFeaturesSystemComponent::m_atomLevelDefaultAssetPath);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorCommonFeaturesSystemComponent>("AtomEditorCommonFeaturesSystemComponent",
                        "Configures editor- and tool-specific functionality for common render features.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(nullptr, &EditorCommonFeaturesSystemComponent::m_atomLevelDefaultAssetPath, "Atom Level Default Asset Path",
                            "path to the slice the instantiate for a new Atom level")
                        ;
                }
            }
        }

        void EditorCommonFeaturesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EditorCommonFeaturesService"));
        }

        void EditorCommonFeaturesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EditorCommonFeaturesService"));
        }

        void EditorCommonFeaturesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("ThumbnailerService"));
            required.push_back(AZ_CRC_CE("PreviewRendererSystem"));
        }

        void EditorCommonFeaturesSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void EditorCommonFeaturesSystemComponent::Init()
        {
        }

        void EditorCommonFeaturesSystemComponent::Activate()
        {
            m_skinnedMeshDebugDisplay = AZStd::make_unique<SkinnedMeshDebugDisplay>();

            AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusConnect();
            if (auto settingsRegistry{ AZ::SettingsRegistry::Get() }; settingsRegistry != nullptr)
            {
                auto LifecycleCallback = [this](const AZ::SettingsRegistryInterface::NotifyEventArgs&)
                {
                    SetupThumbnails();
                };
                AZ::ComponentApplicationLifecycle::RegisterHandler(*settingsRegistry, m_criticalAssetsHandler,
                    AZStd::move(LifecycleCallback), "CriticalAssetsCompiled");
            }
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        }

        void EditorCommonFeaturesSystemComponent::Deactivate()
        {
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
            m_criticalAssetsHandler = {};
            AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusDisconnect();

            m_skinnedMeshDebugDisplay.reset();
            TeardownThumbnails();
        }

        const AzToolsFramework::AssetBrowser::PreviewerFactory* EditorCommonFeaturesSystemComponent::GetPreviewerFactory(
            const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
        {
            return m_previewerFactory->IsEntrySupported(entry) ? m_previewerFactory.get() : nullptr;
        }

        void EditorCommonFeaturesSystemComponent::OnApplicationAboutToStop()
        {
            TeardownThumbnails();
        }

        void EditorCommonFeaturesSystemComponent::SetupThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;
            using namespace LyIntegration;

            ThumbnailerRequestBus::Broadcast(
                &ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(SharedThumbnailCache));

            if (!m_thumbnailRenderer)
            {
                m_thumbnailRenderer = AZStd::make_unique<AZ::LyIntegration::SharedThumbnailRenderer>();
            }

            if (!m_previewerFactory)
            {
                m_previewerFactory = AZStd::make_unique<LyIntegration::SharedPreviewerFactory>();
            }
        }

        void EditorCommonFeaturesSystemComponent::TeardownThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;
            using namespace LyIntegration;

            ThumbnailerRequestBus::Broadcast(&ThumbnailerRequests::UnregisterThumbnailProvider, SharedThumbnailCache::ProviderName);

            m_thumbnailRenderer.reset();
            m_previewerFactory.reset();
        }
    } // namespace Render
} // namespace AZ
