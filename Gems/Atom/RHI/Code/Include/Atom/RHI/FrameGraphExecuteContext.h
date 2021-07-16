/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/ScopeId.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraph;
        class CommandList;

        /**
         * FrameGraphExecuteContext provides a scope-local context for accessing a command list. FrameGraphExecuteContext maps
         * to a single scope and to a single command list. In cases where a scope has been partitioned into N command lists
         * (which is common for platforms which support multi-threaded submission), a scope will map to N execute contexts,
         * where each context is a command list in the batch. Since commands are ordered, each context provides the index of
         * the command list in the batch, as well as the total number of command lists in the batch.
         */
        class FrameGraphExecuteContext
        {
        public:
            struct Descriptor
            {
                ScopeId m_scopeId;
                uint32_t m_commandListIndex = 0;
                uint32_t m_commandListCount = 0;
                CommandList* m_commandList = nullptr;
            };

            FrameGraphExecuteContext(const Descriptor& descriptor);

            /// Returns the scope id associated with this context.
            const ScopeId& GetScopeId() const;

            /// Returns the index of the command list in the batch.
            uint32_t GetCommandListIndex() const;

            /// Returns the total number of command lists in the batch.
            uint32_t GetCommandListCount() const;

            /// Returns the command list associated with the index in the batch.
            CommandList* GetCommandList() const;
            //////////////////////////////////////////////////////////////////////////

            /**
             * Allows setting a command list after initialization (e.g. BeginContextInternal).
             * This is useful if it is preferred to defer command list creation until the context
             * or group begins.
             */
            void SetCommandList(CommandList& commandList);

        private:
            Descriptor m_descriptor;
        };
    }
}
