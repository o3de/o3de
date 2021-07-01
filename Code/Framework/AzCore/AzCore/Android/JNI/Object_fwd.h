/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            namespace Internal
            {
                template<typename Allocator>
                class Object;
            }

            //! \brief The default \ref AZ::Android::JNI::Internal::Object type which uses the SystemAllocator
            typedef Internal::Object<AZ::SystemAllocator> Object;
        }
    }
}
