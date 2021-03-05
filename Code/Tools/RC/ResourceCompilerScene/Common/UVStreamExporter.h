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
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>

namespace AZ
{
    namespace RC
    {
        struct MeshNodeExportContext;

        class UVStreamExporter
            : public SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(UVStreamExporter, "{3840C94B-C131-4C34-B35B-C8E8CFC5AFD1}", SceneAPI::SceneCore::RCExportingComponent);

            UVStreamExporter();
            ~UVStreamExporter() override = default;

            static void Reflect(ReflectContext* context);

            SceneAPI::Events::ProcessingResult CopyUVStream(MeshNodeExportContext& context) const;
        protected:
            SceneAPI::Events::ProcessingResult PopulateUVStream(MeshNodeExportContext& context, int index, AZStd::shared_ptr<const SceneAPI::DataTypes::IMeshVertexUVData> uvs) const;
            static const size_t s_uvMaxStreamCount = 2;
        };
    } // namespace RC
} // namespace AZ