/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // ThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        void ThumbnailKey::SetReady(bool ready)
        {
            m_ready = ready;
            QTimer::singleShot(0, this, &ThumbnailKey::ThumbnailUpdated);
        }

        bool ThumbnailKey::IsReady() const
        {
            return m_ready;
        }

        bool ThumbnailKey::UpdateThumbnail()
        {
            if (!IsReady())
            {
                return false;
            }
            emit ThumbnailUpdateRequested();
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
        }

        Thumbnail::~Thumbnail()
        {
        }

        bool Thumbnail::operator==(const Thumbnail& other) const
        {
            return m_key == other.m_key;
        }

        void Thumbnail::Load()
        {
        }

        void Thumbnail::QueueThumbnailUpdated()
        {
            QTimer::singleShot(0, this, &Thumbnail::ThumbnailUpdated);
            m_lastTimeUpdated = AZStd::chrono::steady_clock::now();
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

        AZStd::chrono::steady_clock::time_point Thumbnail::GetLastTimeUpdated() const
        {
            return m_lastTimeUpdated;
        }

        bool Thumbnail::CanAttemptReload() const
        {
            return AZStd::chrono::steady_clock::now() > m_lastTimeUpdated + AZStd::chrono::milliseconds(5000);
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_Thumbnail.cpp"
