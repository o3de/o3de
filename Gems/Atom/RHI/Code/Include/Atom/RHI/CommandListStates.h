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
