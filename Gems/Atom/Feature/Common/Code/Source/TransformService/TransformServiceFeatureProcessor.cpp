/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Utils/Utils.h>

#include <AzCore/Debug/EventTrace.h>
#include <cinttypes>

namespace AZ
{
    namespace Render
    {
        constexpr size_t BufferReserveCount = 1024;

        void TransformServiceFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<TransformServiceFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void TransformServiceFeatureProcessor::Activate()
        {
            m_updateSceneSrgHandler = RPI::Scene::PrepareSceneSrgEvent::Handler([this](RPI::ShaderResourceGroup *sceneSrg) { this->UpdateSceneSrg(sceneSrg); });
            GetParentScene()->ConnectEvent(m_updateSceneSrgHandler);

            m_deviceBufferNeedsUpdate = true;
            m_objectToWorldTransforms.reserve(BufferReserveCount);
            m_objectToWorldInverseTransposeTransforms.reserve(BufferReserveCount);            

            m_isWriteable = true;

            RPI::SceneNotificationBus::Handler::BusConnect(GetParentScene()->GetId());
        }

        void TransformServiceFeatureProcessor::Deactivate()
        {
            m_objectToWorldTransforms = {};
            m_objectToWorldInverseTransposeTransforms = {};

            m_objectToWorldBuffer = nullptr;
            m_objectToWorldInverseTransposeBuffer = nullptr;
            m_objectToWorldHistoryBuffer = nullptr;

            m_firstAvailableTransformIndex = NoAvailableTransformIndices;

            m_objectToWorldBufferIndex.Reset();
            m_objectToWorldInverseTransposeBufferIndex.Reset();
            m_objectToWorldHistoryBufferIndex.Reset();

            m_isWriteable = false;

            RPI::SceneNotificationBus::Handler::BusDisconnect();
            m_updateSceneSrgHandler.Disconnect();
        }
        
        void TransformServiceFeatureProcessor::PrepareBuffers()
        {
            AZ_Assert(!m_isWriteable, "Must be called between OnBeginPrepareRender() and OnEndPrepareRender()");

            RHI::BufferDescriptor desc;
            desc.m_bindFlags = RHI::BufferBindFlags::ShaderRead;

            {
                const uint32_t elementCount = RHI::NextPowerOfTwo(GetMax<uint32_t>(1, static_cast<uint32_t>(m_objectToWorldTransforms.size())));
                static const uint32_t elementSize = TransformValueSize;
                const uint32_t byteCount = elementCount * TransformValueSize;

                // Create or resize
                if (!m_objectToWorldBuffer)
                {
                    // Create the transform buffer, grow by powers of two
                    RPI::CommonBufferDescriptor desc2;
                    desc2.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc2.m_bufferName =  "m_objectToWorldBuffer";
                    desc2.m_byteCount = byteCount;
                    desc2.m_elementSize = elementSize;

                    m_objectToWorldBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc2);

                    desc2.m_bufferName = "m_objectToWorldHistoryBuffer";
                    m_objectToWorldHistoryBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc2);
                }
                else
                {
                    if (byteCount > m_objectToWorldBuffer->GetBufferSize())
                    {
                        m_objectToWorldBuffer->Resize(byteCount);
                        m_objectToWorldHistoryBuffer->Resize(byteCount);
                    }
                }
            }

            {
                const uint32_t elementCount = RHI::NextPowerOfTwo(GetMax<uint32_t>(1, static_cast<uint32_t>(m_objectToWorldInverseTransposeTransforms.size())));
                static const uint32_t elementSize = NormalValueSize;
                const uint32_t byteCount = elementCount * elementSize;

                // Create or resize
                if (!m_objectToWorldInverseTransposeBuffer)
                {
                    // Create the normal buffer, grow by powers of two
                    RPI::CommonBufferDescriptor desc2;
                    desc2.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                    desc2.m_bufferName = "m_objectToWorldInverseTransposeBuffer";
                    desc2.m_byteCount = byteCount;
                    desc2.m_elementSize = elementSize;

                    m_objectToWorldInverseTransposeBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc2);
                }
                else
                {
                    if (byteCount > m_objectToWorldInverseTransposeBuffer->GetBufferSize())
                    {
                        m_objectToWorldInverseTransposeBuffer->Resize(byteCount);
                    }
                }
            }
        }

        void TransformServiceFeatureProcessor::UpdateSceneSrg(RPI::ShaderResourceGroup *sceneSrg)
        {
            sceneSrg->SetBufferView(m_objectToWorldBufferIndex, m_objectToWorldBuffer->GetBufferView());
            sceneSrg->SetBufferView(m_objectToWorldInverseTransposeBufferIndex, m_objectToWorldInverseTransposeBuffer->GetBufferView());
            sceneSrg->SetBufferView(m_objectToWorldHistoryBufferIndex, m_objectToWorldHistoryBuffer->GetBufferView());
        }

        void TransformServiceFeatureProcessor::OnBeginPrepareRender()
        {
            m_isWriteable = false;

            if (m_historyBufferNeedsUpdate || m_deviceBufferNeedsUpdate)
            {
                PrepareBuffers();
                if (m_historyBufferNeedsUpdate)
                {
                    m_objectToWorldHistoryBuffer->UpdateData(m_objectToWorldHistoryTransforms.data(), m_objectToWorldHistoryTransforms.size() * TransformValueSize);
                    m_historyBufferNeedsUpdate = false;
                }

                if (m_deviceBufferNeedsUpdate)
                {
                    // copy data to the buffers
                    m_objectToWorldBuffer->UpdateData(m_objectToWorldTransforms.data(), m_objectToWorldTransforms.size() * TransformValueSize);
                    m_objectToWorldInverseTransposeBuffer->UpdateData(m_objectToWorldInverseTransposeTransforms.data(), m_objectToWorldInverseTransposeTransforms.size() * NormalValueSize);

                    m_objectToWorldHistoryTransforms = m_objectToWorldTransforms;

                    m_deviceBufferNeedsUpdate = false;
                    m_historyBufferNeedsUpdate = true;
                }
            }
        }

        void TransformServiceFeatureProcessor::OnEndPrepareRender()
        {
            m_isWriteable = true;
        }

        TransformServiceFeatureProcessor::ObjectId TransformServiceFeatureProcessor::ReserveObjectId()
        {
            AZ_Error("TransformServiceFeatureProcessor", m_isWriteable, "Transform data cannot be written to during this phase");
            uint32_t modelIndex = 0;
            if (m_firstAvailableTransformIndex != NoAvailableTransformIndices)
            {
                modelIndex = m_firstAvailableTransformIndex;
                m_firstAvailableTransformIndex = m_objectToWorldTransforms.at(m_firstAvailableTransformIndex).m_nextFreeSlot;
            }
            else
            {
                modelIndex = aznumeric_cast<uint32_t>(m_objectToWorldTransforms.size());
                m_objectToWorldTransforms.push_back();
                m_objectToWorldInverseTransposeTransforms.push_back();
                m_objectToWorldHistoryTransforms.push_back();
            }
            return ObjectId(modelIndex);
        }

        void TransformServiceFeatureProcessor::ReleaseObjectId(ObjectId& id)
        {
            AZ_Error("TransformServiceFeatureProcessor", m_isWriteable, "Transform data cannot be written to during this phase");
            AZ_Error("TransformServiceFeatureProcessor", id.IsValid(), "Attempting to release an invalid handle.");
            if (id.IsValid())
            {
                m_objectToWorldTransforms.at(id.GetIndex()).m_nextFreeSlot = m_firstAvailableTransformIndex;
                m_firstAvailableTransformIndex = id.GetIndex();
                id.Reset();
            }
        }

        void TransformServiceFeatureProcessor::SetTransformForId(ObjectId id, const AZ::Transform& transform, const AZ::Vector3& nonUniformScale)
        {
            AZ_Error("TransformServiceFeatureProcessor", m_isWriteable, "Transform data cannot be written to during this phase");
            AZ_Error("TransformServiceFeatureProcessor", id.IsValid(), "Attempting to set the transform for an invalid handle.");
            if (id.IsValid())
            {
                AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(transform);
                matrix3x4.MultiplyByScale(nonUniformScale);
                matrix3x4.StoreToRowMajorFloat12(m_objectToWorldTransforms.at(id.GetIndex()).m_transform);

                // Inverse transpose to take the non-uniform scale out of the transform for usage with normals.
                matrix3x4.GetInverseFull().GetTranspose3x3().StoreToRowMajorFloat12(m_objectToWorldInverseTransposeTransforms.at(id.GetIndex()).m_transform);
                m_deviceBufferNeedsUpdate = true;
            }
        }

        AZ::Transform TransformServiceFeatureProcessor::GetTransformForId(ObjectId id) const
        {
            AZ_Error("TransformServiceFeatureProcessor", id.IsValid(), "Attempting to get the transform for an invalid handle.");
            AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromRowMajorFloat12(m_objectToWorldTransforms.at(id.GetIndex()).m_transform);
            AZ::Transform transform = AZ::Transform::CreateFromMatrix3x4(matrix3x4);
            transform.ExtractUniformScale();
            return transform;
        }

        AZ::Vector3 TransformServiceFeatureProcessor::GetNonUniformScaleForId(ObjectId id) const
        {
            AZ_Error("TransformServiceFeatureProcessor", id.IsValid(), "Attempting to get the non-uniform scale for an invalid handle.");
            AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromRowMajorFloat12(m_objectToWorldTransforms.at(id.GetIndex()).m_transform);
            return matrix3x4.RetrieveScale();
        }
    }
}
