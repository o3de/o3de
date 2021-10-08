/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>

namespace AtomToolsFramework
{
    //! Provides custom rendering of previefw images
    class PreviewContent
    {
    public:
        AZ_CLASS_ALLOCATOR(PreviewContent, AZ::SystemAllocator, 0);

        PreviewContent() = default;
        virtual ~PreviewContent() = default;
        virtual void Load() = 0;
        virtual bool IsReady() const = 0;
        virtual bool IsError() const = 0;
        virtual void ReportErrors() = 0;
        virtual void UpdateScene() = 0;
    };
} // namespace AtomToolsFramework
