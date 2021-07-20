/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderDataMappings.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        //! Base class for custom data for Passes to be specified in a PassRequest or PassTemplate.
        //! If custom data is specified in both the PassTemplate and the PassRequest, the data
        //! specified in the PassRequest will take precedent and the data in PassTemplate ignored.
        //! All classes for custom pass data must inherit from this or one of it's derived classes.
        struct PassData
        {
            AZ_RTTI(PassData, "{F8594AE8-2588-4D64-89E5-B078A46A9AE4}");
            AZ_CLASS_ALLOCATOR(PassData, SystemAllocator, 0);

            PassData() = default;
            virtual ~PassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<PassData>()
                        ->Version(0);
                }
            }
        };
    } // namespace RPI
} // namespace AZ

