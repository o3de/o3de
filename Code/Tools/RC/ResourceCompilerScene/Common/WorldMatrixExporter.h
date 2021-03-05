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

#include <Cry_Matrix34.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    class Transform;

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IOriginRule;
            class IGroup;
        }
    }

    namespace RC
    {
        struct ContainerExportContext;
        struct NodeExportContext;

        class WorldMatrixExporter
            : public SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(WorldMatrixExporter, "{65A0914C-5953-405F-819B-0E6EB96938F1}", SceneAPI::SceneCore::RCExportingComponent);

            WorldMatrixExporter();
            ~WorldMatrixExporter() override = default;

            static void Reflect(ReflectContext* context);

            SceneAPI::Events::ProcessingResult ProcessMeshGroup(ContainerExportContext& context);
            SceneAPI::Events::ProcessingResult ProcessNode(NodeExportContext& context);

        protected:
            using HierarchyStorageIterator = SceneAPI::Containers::SceneGraph::HierarchyStorageConstIterator;
            using MatrixType = SceneAPI::DataTypes::MatrixType;

            bool ConcatenateMatricesUpwards(MatrixType& transform, const HierarchyStorageIterator& nodeIterator, const SceneAPI::Containers::SceneGraph& graph) const;
            bool MultiplyEndPointTransforms(MatrixType& transform, const HierarchyStorageIterator& nodeIterator, const SceneAPI::Containers::SceneGraph& graph) const;
            void SceneAPIMatrixTypeToMatrix34(Matrix34& out, const MatrixType& in) const;

            MatrixType m_cachedRootMatrix;
            const SceneAPI::DataTypes::IGroup* m_cachedGroup;
            bool m_cachedRootMatrixIsSet;
        };
    } // namespace RC
} // namespace AZ
