/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/LoadingThumbnail.h>
#include <AzToolsFramework/Thumbnails/MissingThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>

#include <QApplication>
#include <QStyle>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        ThumbnailerComponent::ThumbnailerComponent()
            : m_missingThumbnail(new MissingThumbnail())
            , m_loadingThumbnail(new LoadingThumbnail())
            , m_placeholderObject(new QObject())
        {
        }

        ThumbnailerComponent::~ThumbnailerComponent()
        {
            Cleanup();
        }

        void ThumbnailerComponent::Activate()
        {
            ThumbnailerRequestBus::Handler::BusConnect();
        }

        void ThumbnailerComponent::Deactivate()
        {
            Cleanup();
        }

        void ThumbnailerComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ThumbnailerComponent, AZ::Component>();
            }
        }

        void ThumbnailerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ThumbnailerService"));
        }

        void ThumbnailerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ThumbnailerService"));
        }

        void ThumbnailerComponent::RegisterThumbnailProvider(SharedThumbnailProvider provider)
        {
            auto it = AZStd::find_if(m_providers.begin(), m_providers.end(), [provider](const SharedThumbnailProvider& existingProvider)
                {
                    return AZ::StringFunc::Equal(provider->GetProviderName(), existingProvider->GetProviderName());
                });

            if (it != m_providers.end())
            {
                AZ_Error("ThumbnailerComponent", false, "Provider with name %s is already registered with context.", provider->GetProviderName());
                return;
            }

            m_providers.insert(provider);
        }

        void ThumbnailerComponent::UnregisterThumbnailProvider(const char* providerName)
        {
            AZStd::erase_if(
                m_providers,
                [providerName](const SharedThumbnailProvider& provider)
                {
                    return AZ::StringFunc::Equal(provider->GetProviderName(), providerName);
                });

        }

        SharedThumbnail ThumbnailerComponent::GetThumbnail(SharedThumbnailKey key)
        {
            // find provider who can handle supplied key
            for (auto& provider : m_providers)
            {
                SharedThumbnail thumbnail;
                if (provider->GetThumbnail(key, thumbnail))
                {
                    // if thumbnail is ready return it
                    if (thumbnail->GetState() == Thumbnail::State::Ready)
                    {
                        return thumbnail;
                    }

                    if (thumbnail->GetState() == Thumbnail::State::Failed && !thumbnail->CanAttemptReload())
                    {
                        return m_missingThumbnail;
                    }

                    // If a thumbnail is loading or the max number of jobs has been reached then return the loading image.
                    if (thumbnail->GetState() == Thumbnail::State::Loading || m_thumbnailsBeingLoaded.contains(thumbnail))
                    {
                        return m_loadingThumbnail;
                    }

                    // If the thumbnail is not loaded then schedule it to be loaded using a job. Signals will be sent once the load is
                    // complete to update the image.
                    if (thumbnail->GetState() == Thumbnail::State::Unloaded)
                    {
                        // Connect thumbnailer component to the busy label repaint signal to notify the asset browser as it changes. 
                        AzQtComponents::StyledBusyLabel* busyLabel = {};
                        AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(busyLabel, &AssetBrowser::AssetBrowserComponentRequests::GetStyledBusyLabel);
                        QObject::connect(busyLabel, &AzQtComponents::StyledBusyLabel::repaintNeeded, m_placeholderObject.get(), [](){
                            AssetBrowser::AssetBrowserViewRequestBus::Broadcast(&AssetBrowser::AssetBrowserViewRequests::Update);
                        });

                        // The ThumbnailUpdated signal should be sent whenever the thumbnail has loaded or failed. In both cases,
                        // disconnect from all of the signals.
                        QObject::connect(thumbnail.data(), &Thumbnail::ThumbnailUpdated, m_placeholderObject.get(), [this, key, thumbnail, busyLabel]()
                            {
                                QObject::disconnect(busyLabel, nullptr, m_placeholderObject.get(), nullptr);
                                QObject::disconnect(thumbnail.data(), nullptr, key.data(), nullptr);

                                QObject::connect(thumbnail.data(), &Thumbnail::ThumbnailUpdated, key.data(), &ThumbnailKey::ThumbnailUpdated);
                                QObject::connect(key.data(), &ThumbnailKey::ThumbnailUpdateRequested, thumbnail.data(), &Thumbnail::Update);

                                key->SetReady(true);
                                m_thumbnailsBeingLoaded.erase(thumbnail);
                                AssetBrowser::AssetBrowserViewRequestBus::Broadcast(&AssetBrowser::AssetBrowserViewRequests::Update);
                            });

                        // Add the thumbnail to the set of thumbnails in progress then queue the job to load it. The job will send the
                        // ThumbnailUpdated signal from the main thread when complete.
                        m_thumbnailsBeingLoaded.insert(thumbnail);
                        auto job = AZ::CreateJobFunction([thumbnail](){ thumbnail->Load(); }, true);
                        job->Start();
                    }

                    return m_loadingThumbnail;
                }
            }

            return m_missingThumbnail;
        }

        bool ThumbnailerComponent::IsLoading(SharedThumbnailKey key)
        {
            for (auto& provider : m_providers)
            {
                SharedThumbnail thumbnail;
                if (provider->GetThumbnail(key, thumbnail))
                {
                    return thumbnail->GetState() == Thumbnail::State::Unloaded || thumbnail->GetState() == Thumbnail::State::Loading;
                }
            }
            return false;
        }

        void ThumbnailerComponent::Cleanup()
        {
            ThumbnailerRequestBus::Handler::BusDisconnect();

            m_providers.clear();
            m_missingThumbnail.reset();
            m_loadingThumbnail.reset();
            m_placeholderObject.reset();
            m_thumbnailsBeingLoaded.clear();
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

