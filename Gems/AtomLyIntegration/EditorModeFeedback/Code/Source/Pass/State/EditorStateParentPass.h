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
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ::Render
{
    //! Parent pass for editor state effect parent pass classes.
    class EditorStateParentPass : public AZ::RPI::ParentPass
    {
    public:
        AZ_RTTI(EditorStateParentPass, "{C66D2D82-B1A2-4CDF-8E4A-C5C733F34E32}", AZ::RPI::ParentPass);
        AZ_CLASS_ALLOCATOR(EditorStateParentPass, SystemAllocator);

        virtual ~EditorStateParentPass() = default;

        //! Creates a EditorModeFeedbackParentPass
        static RPI::Ptr<EditorStateParentPass> Create(const RPI::PassDescriptor& descriptor);

    protected:
        EditorStateParentPass(const RPI::PassDescriptor& descriptor);

        //! Enables/disables the editor state effect parent pass instance.
        bool IsEnabled() const override;

    private:
        RPI::PassDescriptor m_passDescriptor;
    };
} // namespace AZ::Render
