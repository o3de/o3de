/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! This feature processor handles static and dynamic non-skinned meshes.
        class TransformServiceFeatureProcessor final
            : public TransformServiceFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(TransformServiceFeatureProcessor, AZ::SystemAllocator)

            AZ_RTTI(AZ::Render::TransformServiceFeatureProcessor, "{D8A2C353-2850-42F8-AA21-3979CBECBF80}", AZ::Render::TransformServiceFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            TransformServiceFeatureProcessor() = default;
            virtual ~TransformServiceFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            //! Creates pools, buffers, and buffer views
            void Activate() override;
            //! Releases GPU resources.
            void Deactivate() override;

            // RPI::SceneNotificationBus overrides ...
            void OnBeginPrepareRender() override;
            void OnEndPrepareRender() override;

            // TransformServiceFeatureProcessorInterface overrides ...
            ObjectId ReserveObjectId() override;
            void ReleaseObjectId(ObjectId& id) override;
            void SetTransformForId(ObjectId id, const AZ::Transform& transform,
                const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne()) override;
            AZ::Transform GetTransformForId(ObjectId id) const override;
            AZ::Vector3 GetNonUniformScaleForId(ObjectId id) const override;

        private:

            // Holds both regular 4x3 transforms and 3x3 normal transforms with padding at the end of each float3.
            union Float4x3
            {
                float m_transform[12] = { 0.0f };
                uint32_t m_nextFreeSlot;
            };

            // Flag value for when the buffers have no empty spaces.
            static const uint32_t NoAvailableTransformIndices = std::numeric_limits<uint32_t>::max();

            TransformServiceFeatureProcessor(const TransformServiceFeatureProcessor&) = delete;

            // Prepare GPU buffers for object transformation matrices
            // Create the buffers if they don't exist. Otherwise, resize them if they are not large enough for the matrices
            void PrepareBuffers();

            void UpdateSceneSrg(RPI::ShaderResourceGroup *sceneSrg);

            RPI::Scene::PrepareSceneSrgEvent::Handler m_updateSceneSrgHandler;
            RHI::ShaderInputNameIndex m_objectToWorldBufferIndex = "m_objectToWorldBuffer";
            RHI::ShaderInputNameIndex m_objectToWorldInverseTransposeBufferIndex = "m_objectToWorldInverseTransposeBuffer";
            RHI::ShaderInputNameIndex m_objectToWorldHistoryBufferIndex = "m_objectToWorldHistoryBuffer";

            // Stores transforms that are uploaded to a GPU buffer. Used slots have float12(matrix3x4) values, empty slots
            // have a uint32_t that points to the next empty slot like a linked list. m_firstAvailableMeshTransformIndex stores the first
            // empty slot, unless there are none then it's NoAvailableTransformIndices. This allows mesh object SRGs to be compiled once
            // with an index to their transform, and updates to the transform just update the buffer, not individual mesh SRGs.
            AZStd::vector<Float4x3> m_objectToWorldTransforms;
            AZStd::vector<Float4x3> m_objectToWorldInverseTransposeTransforms;
            AZStd::vector<Float4x3> m_objectToWorldHistoryTransforms;

            static const size_t TransformValueSize = sizeof(decltype(m_objectToWorldTransforms)::value_type);
            static const size_t NormalValueSize = sizeof(decltype(m_objectToWorldInverseTransposeTransforms)::value_type);

            Data::Instance<RPI::Buffer> m_objectToWorldBuffer;
            Data::Instance<RPI::Buffer> m_objectToWorldInverseTransposeBuffer;
            Data::Instance<RPI::Buffer> m_objectToWorldHistoryBuffer;

            uint32_t m_firstAvailableTransformIndex = NoAvailableTransformIndices;
            bool m_deviceBufferNeedsUpdate = false;
            bool m_historyBufferNeedsUpdate = false;
            bool m_isWriteable = true;     //prevents write access during certain parts of the frame (for threadsafety)
        };
    }
}
