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

#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

struct CryBoneDescData;
struct CSkinningInfo;

namespace AZ
{
    namespace RC
    {
        struct SkeletonExportContext;
        struct ResolveRootBoneFromBoneContext;
        struct BuildBoneMapContext;
        struct AddBonesToSkinningInfoContext;

        class SkeletonExporter
            : public SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(SkeletonExporter, "{FDEC2360-3D9C-4027-BCFB-E8C99CAADB43}", SceneAPI::SceneCore::RCExportingComponent);

            SkeletonExporter();
            ~SkeletonExporter() override = default;

            static void Reflect(ReflectContext* context);

            SceneAPI::Events::ProcessingResult ResolveRootBoneFromBone(ResolveRootBoneFromBoneContext& context);
            SceneAPI::Events::ProcessingResult BuildBoneMap(BuildBoneMapContext& context);
            SceneAPI::Events::ProcessingResult AddBonesToSkinningInfo(AddBonesToSkinningInfoContext& context);
            SceneAPI::Events::ProcessingResult ProcessSkeleton(SkeletonExportContext& context);
            
        protected:
            bool AddBonesToSkinningInfo(CSkinningInfo& skinningInfo,
                const AZ::SceneAPI::Containers::SceneGraph& graph, const AZStd::string& rootBoneName) const;
            bool BuildBoneMap(AZStd::unordered_map<AZStd::string, int>& boneNameIdMap, 
                const AZ::SceneAPI::Containers::SceneGraph& graph, const AZStd::string& rootBoneName) const;
            void AddBoneDescriptor(CSkinningInfo& skinningInfo, const char* boneName, size_t boneNameLength,
                const SceneAPI::DataTypes::MatrixType& worldTransform) const;
            bool AddBoneEntity(CSkinningInfo& skinningInfo, const AZ::SceneAPI::Containers::SceneGraph& graph,
                const AZ::SceneAPI::Containers::SceneGraph::NodeIndex index, const AZStd::unordered_map<AZStd::string, int>& boneNameIdMap,
                const char* boneName, const char* bonePath, const AZStd::string& rootBoneName) const;
            void SetBoneName(const char* name, size_t nameLength, CryBoneDescData& boneDesc) const;
        };
    } // namespace RC
} // namespace AZ
