/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the EnvironmentCubeMapPass. Should be specified in the PassRequest.
        struct EnvironmentCubeMapPassData
            : public PassData
        {            
            AZ_RTTI(EnvironmentCubeMapPassData, "{B97DDC6F-1CC6-44D8-8926-042C20D66272}", PassData);
            AZ_CLASS_ALLOCATOR(EnvironmentCubeMapPassData, SystemAllocator, 0);

            EnvironmentCubeMapPassData() = default;
            virtual ~EnvironmentCubeMapPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<EnvironmentCubeMapPassData, PassData>()
                        ->Version(0)
                        ->Field("Position", &EnvironmentCubeMapPassData::m_position)
                        ;
                }
            }

            //! World space position to render the environment cubemap
            Vector3 m_position;
        };
    } // namespace RPI
} // namespace AZ

