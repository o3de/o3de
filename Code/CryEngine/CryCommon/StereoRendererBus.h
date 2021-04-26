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
