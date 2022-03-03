/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the SlowClearPass. Should be specified in the PassRequest.
        struct SlowClearPassData
            : public RenderPassData
        {
            AZ_RTTI(SlowClearPassData, "{5F2C24A4-62D0-4E60-91EC-C207C10D15C6}", RenderPassData);
            AZ_CLASS_ALLOCATOR(SlowClearPassData, SystemAllocator, 0);

            SlowClearPassData() = default;
            virtual ~SlowClearPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<SlowClearPassData, RenderPassData>()
                        ->Version(0)
                        ->Field("ClearValue", &SlowClearPassData::m_clearValue)
                        ;
                }
            }

            RHI::ClearValue m_clearValue;
        };

    } // namespace RPI
} // namespace AZ

