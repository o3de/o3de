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

        class BlendShapeExporter
            : public SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(BlendShapeExporter, "{1A27BF62-F684-4F9E-B2C6-B15E728659EA}", SceneAPI::SceneCore::RCExportingComponent);
            
            BlendShapeExporter();
            ~BlendShapeExporter() override = default;

            static void Reflect(ReflectContext* context);

            SceneAPI::Events::ProcessingResult ProcessBlendShapes(MeshNodeExportContext& context);
        };
    } // namespace RC
} // namespace AZ