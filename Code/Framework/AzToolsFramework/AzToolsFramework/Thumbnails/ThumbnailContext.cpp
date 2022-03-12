/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/Thumbnails/MissingThumbnail.h>
#include <AzToolsFramework/Thumbnails/LoadingThumbnail.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4251: 'QImageIOHandler::d_ptr': class 'QScopedPointer<QImageIOHandlerPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QImageIOHandler'
#include <AzQtComponents/Components/StyledBusyLabel.h>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        ThumbnailContext::ThumbnailContext()
            : m_missingThumbnail(new MissingThumbnail())
            , m_loadingThumbnail(new LoadingThumbnail())
            , m_threadPool(this)
        {
            ThumbnailContextRequestBus::Handler::BusConnect();
        }

        ThumbnailContext::~ThumbnailContext()
        {
            ThumbnailContextRequestBus::Handler::BusDisconnect();
        }

        bool ThumbnailContext::IsLoading(SharedThumbnailKey key)
        {
            SharedThumbnail thumbnail;

            for (auto& provider : m_providers)
            {
                if (provider->GetThumbnail(key, thumbnail))
                {
                    return thumbnail->GetState() == Thumbnail::State::Unloaded ||
                        thumbnail->GetState() == Thumbnail::State::Loading;
                }
            }
            return false;
        }

        void ThumbnailContext::RedrawThumbnail()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Broadcast(&AzToolsFramework::AssetBrowser::AssetBrowserViewRequests::Update);
        }

        QThreadPool* ThumbnailContext::GetThreadPool()
        {
            return &m_threadPool;
        }

        SharedThumbnail ThumbnailContext::GetThumbnail(SharedThumbnailKey key)
        {
            SharedThumbnail thumbnail;
            // find provider who can handle supplied key
            for (auto& provider : m_providers)
            {
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
                        connect(m_loadingThumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                        AzQtComponents::StyledBusyLabel* busyLabel;
                        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(busyLabel, &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::GetStyledBusyLabel);
                        connect(busyLabel, &AzQtComponents::StyledBusyLabel::repaintNeeded, this, &ThumbnailContext::RedrawThumbnail);
                        // once the thumbnail is loaded, disconnect it from loading thumbnail
                        connect(thumbnail.data(), &Thumbnail::Updated, this , [this, key, thumbnail, busyLabel]()
                            {
                                disconnect(m_loadingThumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                                disconnect(busyLabel, &AzQtComponents::StyledBusyLabel::repaintNeeded, this, &ThumbnailContext::RedrawThumbnail);
                                thumbnail->disconnect();
                                connect(thumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                                connect(key.data(), &ThumbnailKey::UpdateThumbnailSignal, thumbnail.data(), &Thumbnail::Update);
                                key->m_ready = true;
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

        void ThumbnailContext::RegisterThumbnailProvider(SharedThumbnailProvider providerToAdd)
        {
            auto it = AZStd::find_if(m_providers.begin(), m_providers.end(), [providerToAdd](const SharedThumbnailProvider& provider)
                {
                    return AZ::StringFunc::Equal(provider->GetProviderName(), providerToAdd->GetProviderName());
                });

            if (it != m_providers.end())
            {
                AZ_Error("ThumbnailContext", false, "Provider with name %s is already registered with context.", providerToAdd->GetProviderName());
                return;
            }

            m_providers.insert(providerToAdd);
        }

        void ThumbnailContext::UnregisterThumbnailProvider(const char* providerName)
        {
            auto it = AZStd::remove_if(m_providers.begin(), m_providers.end(), [providerName](const SharedThumbnailProvider& provider)
                {
                    return AZ::StringFunc::Equal(provider->GetProviderName(), providerName);
                });
            m_providers.erase(it, m_providers.end());
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_ThumbnailContext.cpp"
