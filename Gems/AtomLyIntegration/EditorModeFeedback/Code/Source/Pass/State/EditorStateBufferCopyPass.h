/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Pass/State/EditorStateParentPass.h>
#include <Pass/State/EditorStateParentPassData.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ::Render
{
    //! Pass class for the EditorStateBufferCopy pass to copy over buffer contents.
    class EditorStateBufferCopyPass
        : public AZ::RPI::FullscreenTrianglePass
    {
    public:
        AZ_RTTI(EditorStateBufferCopyPass, "{03EE6F22-7A28-4D01-9D22-0CC04A66B54D}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(EditorStateBufferCopyPass, SystemAllocator);

        virtual ~EditorStateBufferCopyPass() = default;

        //! Creates a EditorStateBufferCopyPass.
        static RPI::Ptr<EditorStateBufferCopyPass> Create(const RPI::PassDescriptor& descriptor);

    protected:
        EditorStateBufferCopyPass(const RPI::PassDescriptor& descriptor);

        //! Enables/disables this copy buffer pass.
        bool IsEnabled() const override;

    private:
        RPI::PassDescriptor m_passDescriptor;
    };
} // namespace AZ::Render
