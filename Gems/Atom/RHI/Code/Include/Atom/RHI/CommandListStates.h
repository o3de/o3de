/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI.Reflect/Scissor.h>

#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    namespace RHI
    {
        template<class T>
        struct CommandListRenderTargetsState
        {
            using StateList = AZStd::fixed_vector<T, RHI::Limits::Pipeline::AttachmentColorCountMax>;
            void Set(AZStd::array_view<T> newElements)
            {
                m_states = StateList(newElements.begin(), newElements.end());
                m_isDirty = true;
            }

            bool IsValid() const
            {
                return !m_states.empty();
            }

            StateList m_states;
            bool m_isDirty = false;
        };

        using CommandListScissorState = CommandListRenderTargetsState<RHI::Scissor>;
        using CommandListViewportState = CommandListRenderTargetsState<RHI::Viewport>;
    }
}
