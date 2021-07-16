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
#endif

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class MissingThumbnail
            : public Thumbnail
        {
            Q_OBJECT
        public:
            MissingThumbnail();
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
