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

#include <AzCore/EBus/EBus.h>

struct DisplayContext;

namespace AzFramework
{
    /// Requests to be made of the DisplayContext (predominantly to Set/Clear the
    /// bound DisplayContext). Prefer to use DisplayContextRequestGuard as opposed
    /// to Set(displayContext)/Set(nullptr) directly.
    class DisplayContextRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SetDC(DisplayContext* displayContext) = 0;
        virtual DisplayContext* GetDC() = 0;

    protected:
        ~DisplayContextRequests() = default;
    };

    /// Inherit from DisplayContextRequestBus::Handler to implement the DisplayContextRequests interface.
    using DisplayContextRequestBus = AZ::EBus<DisplayContextRequests>;

    /// A helper to wrap DisplayContext set EBus calls.
    /// Usage:
    /// // enter scope
    /// {
    ///     AzFramework::DisplayContextRequestGuard displayContextGuard(displayContext);
    ///     ...
    /// } // exit scope, clean up
    /// \attention This is used to bind/unbind DisplayContext for the implementation of
    /// DebugDisplayRequestBus before commands to draw can be used.
    class DisplayContextRequestGuard
    {
    public:
        explicit DisplayContextRequestGuard(DisplayContext& displayContext)
        {
            // store previously bound display context (may be null - not have been bound)
            DisplayContextRequestBus::BroadcastResult(
                m_prevSetDisplayContext, &DisplayContextRequestBus::Events::GetDC);

            // keep track of the display context we are about to set
            m_currSetDisplayContext = &displayContext;

            // set the new display context
            DisplayContextRequestBus::Broadcast(
                &DisplayContextRequestBus::Events::SetDC, &displayContext);
        }

        ~DisplayContextRequestGuard()
        {
            // check what the currently bound display context is
            DisplayContext* currentDisplayContext = nullptr;
            DisplayContextRequestBus::BroadcastResult(
                currentDisplayContext, &DisplayContextRequestBus::Events::GetDC);

            // ensure the current display context has not been changed out from under us
            AZ_Assert(currentDisplayContext == m_currSetDisplayContext,
                "DisplayContext was changed during lifetime of DisplayRequestDCGuard - "
                "This indicates a logical error.")

            // set previous display context
            DisplayContextRequestBus::Broadcast(
                &DisplayContextRequestBus::Events::SetDC, m_prevSetDisplayContext);
        }

    private:
        DisplayContext* m_prevSetDisplayContext = nullptr;
        DisplayContext* m_currSetDisplayContext = nullptr;
    };
} // namespace AzFramework