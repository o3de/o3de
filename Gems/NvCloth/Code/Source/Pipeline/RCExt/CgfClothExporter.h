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

#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>

namespace AZ
{
    namespace RC
    {
        struct MeshNodeExportContext;
        struct ContainerExportContext;
    }
}

namespace NvCloth
{
    namespace Pipeline
    {
        //! This class processes the Scene graph to export cloth data into CGF.
        class CgfClothExporter
            : public AZ::SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(CgfClothExporter, "{3D7287BB-1109-4220-AC44-AEBA59E03FFF}", AZ::SceneAPI::SceneCore::RCExportingComponent);

            CgfClothExporter();

            static void Reflect(AZ::ReflectContext* context);

            //! Process call at CGF Container level.
            //! This function gets called once per Mesh Group from CGF Group Exporter when it's processing meshes.
            AZ::SceneAPI::Events::ProcessingResult ProcessContainerContext(AZ::RC::ContainerExportContext& context) const;

            //! Process call at Mesh Node level.
            //! This function gets called once per Mesh Node inside a Mesh Group from CGF Group Exporter when it's processing meshes.
            AZ::SceneAPI::Events::ProcessingResult ProcessMeshNodeContext(AZ::RC::MeshNodeExportContext& context) const;
        };
    } // namespace Pipeline
} // namespace NvCloth
