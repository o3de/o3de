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

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            class HairRequests
                : public ComponentBus
            {
            public:
                AZ_RTTI(AZ::Render::HairRequests, "{923D6B94-C6AD-4B03-B8CC-DB7E708FB9F4}");

                /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
                static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
                virtual ~HairRequests() {}

                // [To Do] Adi: add here auto-gen virtual getter and setter functions - matching the interface methods    
            };

            typedef AZ::EBus<HairRequests> HairRequestsBus;
        } // namespace Hair
    } // namespace Render
} // namespace AZ
