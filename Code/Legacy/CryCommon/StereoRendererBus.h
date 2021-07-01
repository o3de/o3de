/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    ///
    /// The Stereo Renderer bus allows other systems to query various properties on the stereo renderer
    ///
    class StereoRendererBus
        : public EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBus Traits
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual ~StereoRendererBus() {}

        ///
        /// Return whether the renderer is rendering to the HMD 
        /// 
        /// @return True if rendering to the HMD
        ///
        virtual bool IsRenderingToHMD() { return false; }

    protected:
    };

    using StereoRendererRequestBus = EBus < StereoRendererBus >;
}
