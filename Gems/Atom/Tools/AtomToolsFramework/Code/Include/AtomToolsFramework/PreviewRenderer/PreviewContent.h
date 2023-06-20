/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AtomToolsFramework
{
    //! Interface for describing scene content that will be rendered using the PreviewRenderer
    class PreviewContent
    {
    public:
        AZ_CLASS_ALLOCATOR(PreviewContent, AZ::SystemAllocator);

        PreviewContent() = default;
        virtual ~PreviewContent() = default;

        //! Initiate loading of scene content, models, materials, etc
        virtual void Load() = 0;

        //! Return true if content is loaded and ready to render
        virtual bool IsReady() const = 0;

        //! Return true if content failed to load
        virtual bool IsError() const = 0;

        //! Report any issues encountered while loading
        virtual void ReportErrors() = 0;

        //! Prepare or pose content before rendering
        virtual void Update() = 0;
    };
} // namespace AtomToolsFramework
