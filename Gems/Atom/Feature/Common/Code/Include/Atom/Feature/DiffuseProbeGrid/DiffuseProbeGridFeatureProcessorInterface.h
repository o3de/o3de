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

#include <AzCore/base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGrid;

        using DiffuseProbeGridHandle = AZStd::shared_ptr<DiffuseProbeGrid>;

        // DiffuseProbeGridFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
        class DiffuseProbeGridFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridFeatureProcessorInterface, "{6EF4F226-D473-4D50-8884-D407E4D145F4}");

            virtual DiffuseProbeGridHandle AddProbeGrid(const AZ::Transform& transform, const AZ::Vector3& extents, const AZ::Vector3& probeSpacing) = 0;
            virtual void RemoveProbeGrid(DiffuseProbeGridHandle& handle) = 0;
            virtual bool IsValidProbeGridHandle(const DiffuseProbeGridHandle& probeGrid) const = 0;
            virtual bool ValidateExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newExtents) = 0;
            virtual void SetExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& extents) = 0;
            virtual void SetTransform(const DiffuseProbeGridHandle& probeGrid, const AZ::Transform& transform) = 0;
            virtual bool ValidateProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newSpacing) = 0;
            virtual void SetProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& probeSpacing) = 0;
            virtual void SetViewBias(const DiffuseProbeGridHandle& probeGrid, float viewBias) = 0;
            virtual void SetNormalBias(const DiffuseProbeGridHandle& probeGrid, float normalBias) = 0;
            virtual void SetAmbientMultiplier(const DiffuseProbeGridHandle& probeGrid, float ambientMultiplier) = 0;
            virtual void Enable(const DiffuseProbeGridHandle& probeGrid, bool enable) = 0;
            virtual void SetGIShadows(const DiffuseProbeGridHandle& probeGrid, bool giShadows) = 0;
        };
    } // namespace Render
} // namespace AZ
