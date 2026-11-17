/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        //! Implemented by anything that owns a cancellable viewport drag.
        //!
        //! "Right click abandons the drag in progress" is standard behaviour in DCC tools and is
        //! worth having, but a tool cannot implement it by watching the mouse itself. While a
        //! manipulator is interacting, ManipulatorManager::ConsumeViewportMousePress returns true
        //! for every press so long as a manipulator is active, so the event is swallowed and never
        //! reaches component modes or viewport tools at all.
        //!
        //! A handler sitting above the manipulator manager - a viewport selection handler, which
        //! sees input first - does see the click, and is therefore the only place that can notice
        //! the right button going down mid-drag. It broadcasts here; each tool reverts whatever
        //! its own drag had done.
        class ViewportDragCancelRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            //! Abandon the drag in progress, restoring whatever it had modified.
            //!
            //! @return True if there was a drag to cancel. The broadcaster consumes the click when
            //! any handler returns true, so a right click that cancelled a drag does not also
            //! start a camera look or open the context menu. Returning true when nothing was
            //! cancelled would swallow ordinary right clicks.
            virtual bool CancelActiveDrag() = 0;

        protected:
            ~ViewportDragCancelRequests() = default;
        };

        using ViewportDragCancelRequestBus = AZ::EBus<ViewportDragCancelRequests>;
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
