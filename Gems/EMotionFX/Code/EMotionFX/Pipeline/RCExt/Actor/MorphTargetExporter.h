/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

