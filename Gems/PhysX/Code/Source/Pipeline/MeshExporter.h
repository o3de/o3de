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

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            class ExportEventContext;
        }

        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IMeshData;
        }
    }
}

namespace PhysX
{
    namespace Pipeline
    {
        class MeshGroup;

        class MeshExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(MeshExporter, "{4EA8B035-064D-456F-A9BA-0CDA40E9B84C}", AZ::SceneAPI::SceneCore::ExportingComponent);

            MeshExporter();
            ~MeshExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(AZ::SceneAPI::Events::ExportEventContext& context) const;

        private:
            AZ::SceneAPI::Events::ProcessingResult ExportMeshObject(AZ::SceneAPI::Events::ExportEventContext& context, const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData>& meshToExport, const AZStd::string& nodePath, const Pipeline::MeshGroup& pxMeshGroup) const;
        };
    }
}
