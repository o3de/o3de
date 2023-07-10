/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

#include <Atom/RHI/FrameGraphAttachmentInterface.h>

namespace AZ::RHI
{
    class ScopeProducer;

    class FrameGraphBuilder
    {
    public:
        virtual ~FrameGraphBuilder() = default;

        //! Returns the frame graph attachment builder, which allows the user to declare global attachments.
        virtual FrameGraphAttachmentInterface GetAttachmentDatabase() = 0;

        //! Imports a scope producer into the frame graph. Scope producers are prepared in the order they are
        //! imported, however the compile phase runs a topological sort based on the attachment and explicit scope
        //! dependencies.
        virtual ResultCode ImportScopeProducer(ScopeProducer& scopeProducer) = 0;
    };
}
