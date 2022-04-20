/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
            , m_threadPool(this)
        {
        }

        ThumbnailerComponent::~ThumbnailerComponent() = default;

        void ThumbnailerComponent::Activate()
        {
            ThumbnailerRequestBus::Handler::BusConnect();
        }

        void ThumbnailerComponent::Deactivate()
        {
            ThumbnailerRequestBus::Handler::BusDisconnect();
            m_providers.clear();
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

                    // if thumbnail is not loaded, start loading it, meanwhile return loading thumbnail
                    if (thumbnail->GetState() == Thumbnail::State::Unloaded)
                    {
                        // listen to the loading signal, so the anyone using it will update loading animation
                        AzQtComponents::StyledBusyLabel* busyLabel;
                        AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(busyLabel, &AssetBrowser::AssetBrowserComponentRequests::GetStyledBusyLabel);
                        QObject::connect(m_loadingThumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                        QObject::connect(busyLabel, &AzQtComponents::StyledBusyLabel::repaintNeeded, this, &ThumbnailerComponent::RedrawThumbnail);

                        // once the thumbnail is loaded, disconnect it from loading thumbnail
                        QObject::connect(thumbnail.data(), &Thumbnail::Updated, this , [this, key, thumbnail, busyLabel]()
                            {
                                QObject::disconnect(m_loadingThumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                                QObject::disconnect(busyLabel, &AzQtComponents::StyledBusyLabel::repaintNeeded, this, &ThumbnailerComponent::RedrawThumbnail);

                                thumbnail->disconnect();
                                QObject::connect(thumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                                QObject::connect(key.data(), &ThumbnailKey::UpdateThumbnailSignal, thumbnail.data(), &Thumbnail::Update);

                                key->SetReady(true);
                                Q_EMIT key->ThumbnailUpdatedSignal();
                            });

                        thumbnail->Load();
                    }

                    if (thumbnail->GetState() == Thumbnail::State::Failed)
                    {
                        return m_missingThumbnail;
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

        QThreadPool* ThumbnailerComponent::GetThreadPool()
        {
            return &m_threadPool;
        }

        void ThumbnailerComponent::RedrawThumbnail()
        {
            AssetBrowser::AssetBrowserViewRequestBus::Broadcast(&AssetBrowser::AssetBrowserViewRequests::Update);
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

