/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Pass/State/EditorStateParentPassBase.h>

#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>

namespace AZ::RPI
{
    //! Custom data for the FullscreenTrianglePass. Should be specified in the PassRequest.
    struct EditorStateParentPassData
        : public RPI::RenderPassData
    {
        AZ_RTTI(EditorStateParentPassData, "{0E0FD1EE-906C-45B5-B65E-463719A90712}", RenderPassData);
        AZ_CLASS_ALLOCATOR(EditorStateParentPassData, SystemAllocator, 0);

        EditorStateParentPassData() = default;
        virtual ~EditorStateParentPassData() = default;

        const Render::EditorStateParentPassBase* editorStatePass = nullptr;
    };

    //! Custom data for the FullscreenTrianglePass. Should be specified in the PassRequest.
    struct EditorStatePassthroughPassData
        : public RPI::FullscreenTrianglePassData
    {
        AZ_RTTI(EditorStatePassthroughPassData, "{3782A63C-4FFE-417B-86B5-C61E986CCBE6}", FullscreenTrianglePassData);
        AZ_CLASS_ALLOCATOR(EditorStatePassthroughPassData, SystemAllocator, 0);

        EditorStatePassthroughPassData() = default;
        virtual ~EditorStatePassthroughPassData() = default;

        const Render::EditorStateParentPassBase* editorStatePass = nullptr;
    };
} // namespace AZ::RPI
