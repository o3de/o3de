/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Pass/State/EditorStateBase.h>

#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>

namespace AZ::RPI
{
    //! Custom data for the EditorStateBufferCopyPass. Should be specified in the PassRequest.
    struct EditorStateBufferCopyPassData
        : public RPI::FullscreenTrianglePassData
    {
        AZ_RTTI(EditorStateBufferCopyPassData, "{3782A63C-4FFE-417B-86B5-C61E986CCBE6}", FullscreenTrianglePassData);
        AZ_CLASS_ALLOCATOR(EditorStateBufferCopyPassData, SystemAllocator);

        EditorStateBufferCopyPassData() = default;
        virtual ~EditorStateBufferCopyPassData() = default;

        //! Pointer to the owning editor state effect parent pass instance.
        const Render::EditorStateBase* editorStatePass = nullptr;
    };
} // namespace AZ::RPI
