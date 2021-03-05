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

        class ColorStreamExporter
            : public SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(ColorStreamExporter, "{912F9D7B-55C1-4871-A3BE-6C63B27E6B49}", SceneAPI::SceneCore::RCExportingComponent);

            ColorStreamExporter();
            ~ColorStreamExporter() override = default;

            static void Reflect(ReflectContext* context);

            SceneAPI::Events::ProcessingResult CopyVertexColorStream(MeshNodeExportContext& context) const;
        };
    } // namespace RC
} // namespace AZ