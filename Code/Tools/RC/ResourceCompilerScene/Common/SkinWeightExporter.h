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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISkinWeightData;
        }
    }

    namespace RC
    {
        struct ResolveRootBoneFromNodeContext;
        struct MeshNodeExportContext;

        class SkinWeightExporter
            : public SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            using BoneNameIdMap = AZStd::unordered_map<AZStd::string, int>;

            AZ_COMPONENT(SkinWeightExporter, "{97C7D185-14F5-4BB1-AAE0-120A722882D1}", SceneAPI::SceneCore::RCExportingComponent);

            SkinWeightExporter();
            ~SkinWeightExporter() override = default;

            static void Reflect(ReflectContext* context);

            SceneAPI::Events::ProcessingResult ResolveRootBoneFromNode(ResolveRootBoneFromNodeContext& context);
            SceneAPI::Events::ProcessingResult ProcessSkinWeights(MeshNodeExportContext& context);
            SceneAPI::Events::ProcessingResult ProcessTouchBendableSkinWeights(TouchBendableMeshNodeExportContext& context);

        protected:
            void SetSkinWeights(MeshNodeExportContext& context, BoneNameIdMap boneNameIdMap);
            int GetGlobalBoneId(const AZStd::shared_ptr<const SceneAPI::DataTypes::ISkinWeightData>& skinWeights, BoneNameIdMap boneNameIdMap, int boneId);
        };
    } // namespace RC
} // namespace AZ