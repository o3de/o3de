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
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IBlendShapeData;
        }
    }
}

namespace EMotionFX
{
    class Node;
    class Actor;

    namespace Pipeline
    {
        class CoordinateSystemConverter;
        struct ActorMorphBuilderContext;
        using SceneAPIMatrixType = AZ::SceneAPI::DataTypes::MatrixType;

        class MorphTargetExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(MorphTargetExporter, "{3B657DB7-1737-40BE-8056-117090965B06}", AZ::SceneAPI::SceneCore::ExportingComponent);

            MorphTargetExporter();
            ~MorphTargetExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessMorphTargets(ActorMorphBuilderContext& context);
        };
    }
} // namespace EMotionFX

