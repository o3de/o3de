/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandList.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<CommandList> CommandList::Create()
        {
            return aznew CommandList();
        }

        void CommandList::Shutdown()
        {
            DeviceObject::Shutdown();
        }
    }
}
