/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    //! A class which manages a FrameCountMax number of objects and manages them in a ring buffer structure, meaning that whenever an
    //! element needs to be updated, the current element index is incremented (mod FrameCountMax), which leaves the other elements
    //! unchanged, which is necessary for some resources if the GPU and CPU are not in sync.
    template<typename T>
    class FrameCountMaxRingBuffer
    {
    private:
        AZStd::array<T, Limits::Device::FrameCountMax> m_elements;
        unsigned m_currentElementIndex{ 0 };

    public:
        //! Increments the current element index (mod FrameCountMax) and returns a reference the new current element. This should happen at
        //! most once per frame.
        T& AdvanceCurrentElement()
        {
            m_currentElementIndex = (m_currentElementIndex + 1) % Limits::Device::FrameCountMax;
            return GetCurrentElement();
        }

        //! Returns the current element
        const T& GetCurrentElement() const
        {
            return m_elements[m_currentElementIndex];
        }

        //! Returns the current element
        T& GetCurrentElement()
        {
            return m_elements[m_currentElementIndex];
        }

        //! Returns the number of elements that are managed by this class
        unsigned GetElementCount() const
        {
            return Limits::Device::FrameCountMax;
        }
    };
} // namespace AZ::RHI
