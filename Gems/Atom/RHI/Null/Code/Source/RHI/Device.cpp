/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Null_precompiled.h"
#include <RHI/Device.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<Device> Device::Create()
        {
            return aznew Device();
        }

        void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
        {
            formatsCapabilities.fill(static_cast<RHI::FormatCapabilities>(~0));
        }
    }
}
