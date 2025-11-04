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

namespace AZ::RHI
{
    //! State of a property that affects the render target attachments in a command list.
    template<class T>
    struct CommandListRenderTargetsState
    {
        using StateList = AZStd::fixed_vector<T, RHI::Limits::Pipeline::AttachmentColorCountMax>;
        void Set(AZStd::span<const T> newElements)
        {
            m_states = StateList(newElements.begin(), newElements.end());
            m_isDirty = true;
        }

        bool IsValid() const
        {
            return !m_states.empty();
        }

        //! List with the state for each render target.
        StateList m_states;
        //! Whether the states have already been applied to the command list.
        bool m_isDirty = false;
    };

    using CommandListScissorState = CommandListRenderTargetsState<RHI::Scissor>;
    using CommandListViewportState = CommandListRenderTargetsState<RHI::Viewport>;

    //! State of the shading rate of a command list.
    struct CommandListShadingRateState
    {
        void Set(ShadingRate rate, const RHI::ShadingRateCombinators& combinators)
        {
            m_shadingRate = rate;
            m_shadingRateCombinators = combinators;
            m_isDirty = true;
        }

        //! Shading rate value
        ShadingRate m_shadingRate = RHI::ShadingRate::Rate1x1;
        //! Shading rate combinator operators
        RHI::ShadingRateCombinators m_shadingRateCombinators = CommandList::DefaultShadingRateCombinators;
        //! Whether the state has already been applied to the command list.
        bool m_isDirty = false;
    };
}
