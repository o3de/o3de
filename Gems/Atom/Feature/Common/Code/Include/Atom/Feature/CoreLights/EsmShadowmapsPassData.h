/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(EsmShadowmapsPassData, SystemAllocator);

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
