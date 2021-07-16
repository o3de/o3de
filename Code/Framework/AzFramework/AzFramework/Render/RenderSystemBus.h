/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    namespace Render
    {
        class RenderSystemRequests
            : public AZ::EBusTraits
        {
        public:

            virtual ~RenderSystemRequests() = default;

            //! Single handler policy since there should only be one instance of this system component.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Get the name of the renderer
            virtual AZStd::string GetRendererName() const = 0;
        };

        using RenderSystemRequestBus = AZ::EBus<RenderSystemRequests>;

    } // namespace Render
} // namespace AzFramework
