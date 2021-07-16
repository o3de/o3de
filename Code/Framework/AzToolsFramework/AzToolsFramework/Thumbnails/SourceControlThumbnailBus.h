/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! Notifications for source control thumbnail
        class SourceControlThumbnailRequests
            : public AZ::EBusTraits
        {
        public:
            //! Notify listeners that file's source control status may have changed
            virtual void FileStatusChanged(const char* filename) = 0;
        };

        using SourceControlThumbnailRequestBus = AZ::EBus<SourceControlThumbnailRequests>;
    } // namespace Thumbnailer
} // namespace AzToolsFramework
