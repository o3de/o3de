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

#include <Atom/RPI.Reflect/Pass/PassData.h>

namespace AZ
{
    namespace Render
    {
        //! Custom data for EsmShadowmapsPass.  Should be specified in the PassRequest.
        struct EsmShadowmapsPassData
            : public RPI::PassData
        {
            using Base = RPI::PassData;
            AZ_RTTI(EsmShadowmapsPassData, "9B2265DF-2234-4DA9-A79F-7D34F6474160", Base);
            AZ_CLASS_ALLOCATOR(EsmShadowmapsPassData, SystemAllocator, 0);

            EsmShadowmapsPassData() = default;
            virtual ~EsmShadowmapsPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<EsmShadowmapsPassData, Base>()
                        ->Version(0)
                        ->Field("LightType", &EsmShadowmapsPassData::m_lightType)
                        ;
                }
            }

            Name m_lightType;
        };
    } // namespace Rendr
} // namespace AZ
