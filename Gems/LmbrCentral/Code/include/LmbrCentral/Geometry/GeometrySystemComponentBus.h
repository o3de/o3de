
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>

namespace LmbrCentral
{
    /**
     * GeometrySystem Requests for generating capsule geometry.
     */
    class CapsuleGeometrySystemRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~CapsuleGeometrySystemRequests() = default;

        /**
         * Generates capsule geometry with lines.
         */
        virtual void GenerateCapsuleMesh(
            float radius,
            float height,
            AZ::u32 sides,
            AZ::u32 capSegments,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut) = 0;
    };

    using CapsuleGeometrySystemRequestBus = AZ::EBus<CapsuleGeometrySystemRequests>;

} // namespace LmbrCentral
