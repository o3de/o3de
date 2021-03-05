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

