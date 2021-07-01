/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
AZ_PUSH_DISABLE_WARNING(4127 4251 4800 4244, "-Wunknown-warning-option") // 4127: conditional expression is constant
                                                                         // 4251: 'QTextCodec::ConverterState::flags': class 'QFlags<QTextCodec::ConversionFlag>' needs to have dll-interface to be used by clients of struct 'QTextCodec::ConverterState'
                                                                         // 4800: 'QTextBoundaryFinderPrivate *const ': forcing value to bool 'true' or 'false' (performance warning)
                                                                         // 4244: conversion from 'int' to 'qint8', possible loss of data
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // ThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        bool ThumbnailKey::IsReady() const { return m_ready; }

        bool ThumbnailKey::UpdateThumbnail()
        {
            if (!IsReady())
            {
                return false;
            }
            emit UpdateThumbnailSignal();
            return true;
        }

        size_t ThumbnailKey::GetHash() const
        {
            return 0;
        }

        bool ThumbnailKey::Equals(const ThumbnailKey* other) const
        {
            return RTTI_GetType() == other->RTTI_GetType();
        }

        //////////////////////////////////////////////////////////////////////////
        // Thumbnail
        //////////////////////////////////////////////////////////////////////////
        Thumbnail::Thumbnail(SharedThumbnailKey key)
            : QObject()
            , m_state(State::Unloaded)
            , m_key(key)
        {
            connect(&m_watcher, &QFutureWatcher<void>::finished, this, [this]()
                {
                    if (m_state == State::Loading)
                    {
                        m_state = State::Ready;
                    }
                    Q_EMIT Updated();
                });
        }

        Thumbnail::~Thumbnail() = default;

        bool Thumbnail::operator==(const Thumbnail& other) const
        {
            return m_key == other.m_key;
        }

        void Thumbnail::Load()
        {
            if (m_state == State::Unloaded)
            {
                m_state = State::Loading;
                QThreadPool* threadPool;
                ThumbnailContextRequestBus::BroadcastResult(
                    threadPool,
                    &ThumbnailContextRequestBus::Handler::GetThreadPool);
                QFuture<void> future = QtConcurrent::run(threadPool, [this](){ LoadThread(); });
                m_watcher.setFuture(future);
            }
        }

        const QPixmap& Thumbnail::GetPixmap() const
        {
            return m_pixmap;
        }

        SharedThumbnailKey Thumbnail::GetKey() const
        {
            return m_key;
        }

        Thumbnail::State Thumbnail::GetState() const
        {
            return m_state;
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_Thumbnail.cpp"
