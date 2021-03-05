#pragma once

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

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>

struct CNodeCGF;
class CContentCGF;

namespace AZ
{
    class ReflectContext;

    namespace RC
    {
        struct CgfGroupExportContext;
        struct ContainerExportContext;
        struct MeshNodeExportContext;

        class TouchBendingExporter
            : public AZ::SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(TouchBendingExporter, "{4C6694B3-F7A8-48D8-A10A-46D57F8CC75E}", AZ::SceneAPI::SceneCore::RCExportingComponent);

            TouchBendingExporter();
            ~TouchBendingExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            SceneAPI::Events::ProcessingResult ConfigureContainer(AZ::RC::ContainerExportContext& context);
            SceneAPI::Events::ProcessingResult ProcessSkinnedMesh(AZ::RC::MeshNodeExportContext& context);

            static constexpr const char * const TraceWindowName = "TouchBending";

        protected:
            /*!
              StaticObjectCompiler, when building SFoliageInfoCGF, uses the "branch%d_%d" named bones to build the spines.
              This methods adds the tree of CNodeCGF helper nodes from Bones with such names.
            */
            bool AddHelperBoneNodes(AZ::RC::ContainerExportContext& context, CContentCGF& content, AZStd::string& rootBoneName,
                bool shouldOverrideDamping,   float damping,
                bool shouldOverrideStiffness, float stiffness,
                bool shouldOverrideThickness, float thickness);
        }; //class TouchBendingCgfExporter
    } // namespace RC
} //namespace AZ
