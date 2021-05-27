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

#include <OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessor.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Culling.h>

namespace AZ
{
    namespace Render
    {
        void OcclusionCullingPlaneFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<OcclusionCullingPlaneFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void OcclusionCullingPlaneFeatureProcessor::Activate()
        {
            m_occlusionCullingPlanes.reserve(InitialOcclusionCullingPlanesAllocationSize);

            EnableSceneNotification();
        }

        void OcclusionCullingPlaneFeatureProcessor::Deactivate()
        {
            AZ_Warning("OcclusionCullingPlaneFeatureProcessor", m_occlusionCullingPlanes.size() == 0,
                "Deactivating the OcclusionCullingPlaneFeatureProcessor, but there are still outstanding occlusion planes. Components\n"
                "using OcclusionCullingPlaneHandles should free them before the OcclusionCullingPlaneFeatureProcessor is deactivated.\n"
            );

            DisableSceneNotification();
        }

        void OcclusionCullingPlaneFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            AZStd::vector<AZ::Transform> occlusionCullingPlanes;
            for (auto& occlusionCullingPlane : m_occlusionCullingPlanes)
            {
                occlusionCullingPlanes.push_back(occlusionCullingPlane->GetTransform());
            }
            GetParentScene()->GetCullingScene()->SetOcclusionCullingPlanes(occlusionCullingPlanes);
        }

        OcclusionCullingPlaneHandle OcclusionCullingPlaneFeatureProcessor::AddOcclusionCullingPlane(const AZ::Transform& transform)
        {
            AZStd::shared_ptr<OcclusionCullingPlane> occlusionCullingPlane = AZStd::make_shared<OcclusionCullingPlane>();
            occlusionCullingPlane->SetTransform(transform);
            m_occlusionCullingPlanes.push_back(occlusionCullingPlane);
            return occlusionCullingPlane;
        }

        void OcclusionCullingPlaneFeatureProcessor::RemoveOcclusionCullingPlane(OcclusionCullingPlaneHandle& occlusionCullingPlane)
        {
            AZ_Assert(occlusionCullingPlane.get(), "RemoveOcclusionCullingPlane called with an invalid handle");

            auto itEntry = AZStd::find_if(m_occlusionCullingPlanes.begin(), m_occlusionCullingPlanes.end(), [&](AZStd::shared_ptr<OcclusionCullingPlane> const& entry)
            {
                return (entry == occlusionCullingPlane);
            });

            AZ_Assert(itEntry != m_occlusionCullingPlanes.end(), "RemoveOcclusionCullingPlane called with an occlusion plane that is not in the occlusion plane list");
            m_occlusionCullingPlanes.erase(itEntry);
            occlusionCullingPlane = nullptr;
        }

        void OcclusionCullingPlaneFeatureProcessor::SetTransform(const OcclusionCullingPlaneHandle& occlusionCullingPlane, const AZ::Transform& transform)
        {
            AZ_Assert(occlusionCullingPlane.get(), "SetTransform called with an invalid handle");
            occlusionCullingPlane->SetTransform(transform);
        }

        void OcclusionCullingPlaneFeatureProcessor::SetEnabled(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool enabled)
        {
            AZ_Assert(occlusionCullingPlane.get(), "Enable called with an invalid handle");
            occlusionCullingPlane->SetEnabled(enabled);
        }
    } // namespace Render
} // namespace AZ
