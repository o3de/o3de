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
