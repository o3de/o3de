/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! Provides custom rendering of preeview images
            class CommonPreviewContent
            {
            public:
                AZ_CLASS_ALLOCATOR(CommonPreviewContent, AZ::SystemAllocator, 0);

                CommonPreviewContent() = default;
                virtual ~CommonPreviewContent() = default;
                virtual void Load() = 0;
                virtual bool IsReady() const = 0;
                virtual bool IsError() const = 0;
                virtual void ReportErrors() = 0;
                virtual void UpdateScene() = 0;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
