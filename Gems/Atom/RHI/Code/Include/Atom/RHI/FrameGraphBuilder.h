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

#include <Atom/RHI.Reflect/Base.h>

#include <Atom/RHI/FrameGraphAttachmentInterface.h>

namespace AZ
{
    namespace RHI
    {
        class ScopeProducer;

        class FrameGraphBuilder
        {
        public:
            virtual ~FrameGraphBuilder() = default;

            /**
             * Returns the frame graph attachment builder, which allows the user to declare global attachments.
             */
            virtual FrameGraphAttachmentInterface GetAttachmentDatabase() = 0;

            /**
             * Imports a scope producer into the frame graph. Scope producers are prepared in the order they are
             * imported, however the compile phase runs a topological sort based on the attachment and explicit scope
             * dependencies.
             */
            virtual ResultCode ImportScopeProducer(ScopeProducer& scopeProducer) = 0;
        };
    }
}
