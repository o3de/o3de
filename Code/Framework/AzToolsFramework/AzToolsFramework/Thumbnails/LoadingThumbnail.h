/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class 'QImageIOHandler::d_ptr': class 'QScopedPointer<QImageIOHandlerPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QImageIOHandler'
#include <QMovie>
#endif
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class LoadingThumbnail
            : public Thumbnail
            , public AZ::TickBus::Handler
        {
            Q_OBJECT
        public:
            LoadingThumbnail();
            ~LoadingThumbnail() override;

            void UpdateTime(float /*deltaTime*/) override;

            //////////////////////////////////////////////////////////////////////////
            // TickBus
            //////////////////////////////////////////////////////////////////////////
            //! LoadingThumbnail is not part pf any thumbnail cache, so it needs to be updated manually
            void OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/) override;

        private:
            QMovie m_loadingMovie;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
