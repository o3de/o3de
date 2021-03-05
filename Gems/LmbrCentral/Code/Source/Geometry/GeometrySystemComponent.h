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
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

namespace LmbrCentral
{
    /**
     * System component for generating geometry.
     */
    class GeometrySystemComponent
        : public AZ::Component
        , public CapsuleGeometrySystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(GeometrySystemComponent, "{53D0A293-63C8-420A-8FEE-B6BFBB804D7A}");
        static void Reflect(AZ::ReflectContext* context);

        GeometrySystemComponent() = default;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GeometryService", 0x5676f10b));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // CapsuleGeometrySystemRequests
        void GenerateCapsuleMesh(
            float radius,
            float height,
            AZ::u32 sides,
            AZ::u32 capSegments,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut) override;
    };
}