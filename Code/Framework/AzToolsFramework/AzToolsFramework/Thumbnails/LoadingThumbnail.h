/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            explicit LoadingThumbnail(int thumbnailSize);
            ~LoadingThumbnail() override;

            void UpdateTime(float /*deltaTime*/) override;

            //////////////////////////////////////////////////////////////////////////
            // TickBus
            //////////////////////////////////////////////////////////////////////////
            //! LoadingThumbnail is not part pf any thumbnail cache, so it needs to be updated manually
            void OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/) override;

        private:
            float m_angle;
            QMovie m_loadingMovie;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
