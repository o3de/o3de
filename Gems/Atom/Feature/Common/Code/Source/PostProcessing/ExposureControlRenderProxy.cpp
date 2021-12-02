/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/ExposureControlRenderProxy.h>
#include <PostProcessing/ExposureControlFeatureProcessor.h>
#include <PostProcessing/EyeAdaptationPass.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperPass.h>

#include <Atom/RPI.Public/View.h>

#include <Atom/RHI/FrameScheduler.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
    namespace Render
    {
        ExposureControlRenderProxy::ExposureControlRenderProxy()
        {
        }

        void ExposureControlRenderProxy::Init(const RPI::ViewPtr view, uint32_t idNumber)
        {
            AZ_Assert(view != nullptr, "Invalid view pointer passed to the exposure control render proxy.");

            if (view != nullptr)
            {
                m_viewPtr = view;
                m_viewSrg = view->GetShaderResourceGroup();

                m_eyeAdaptationBuffer.Init(m_viewSrg, idNumber);
            }

            InitCommonBuffer(idNumber);

        }

        void ExposureControlRenderProxy::Terminate()
        {
            TerminateCommonBuffer();
            m_eyeAdaptationBuffer.Terminate();
            m_viewSrg.reset();
        }


        bool ExposureControlRenderProxy::InitCommonBuffer(uint32_t idNumber)
        {
            AZStd::string bufferName = AZStd::string::format("%s_%d", ExposureControlBufferBaseName, idNumber);
            // Check to see if the buffer previously created already exists.
            // If the old buffer existed, this buffer will be reused to avoid initialization process.
            m_buffer = RPI::BufferSystemInterface::Get()->FindCommonBuffer(bufferName);

            if (m_buffer == nullptr)
            {                
                RPI::CommonBufferDescriptor desc;
                desc.m_poolType = RPI::CommonBufferPoolType::Constant;
                desc.m_bufferName = bufferName;
                desc.m_byteCount = sizeof(ShaderParameters);
                desc.m_elementSize = sizeof(ShaderParameters);
                desc.m_isUniqueName = true;

                m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            }
 
            if (!m_buffer)
            {
                AZ_Assert(false, "Failed to create the RPI::Buffer[%s] which is used for the exposure control feature.", bufferName.c_str());
                return false;
            }

            return true;
        }

        void ExposureControlRenderProxy::TerminateCommonBuffer()
        {
            m_buffer = nullptr;
        }

        void ExposureControlRenderProxy::Simulate(const AZ::RPI::FeatureProcessor::SimulatePacket& packet)
        {
            m_eyeAdaptationBuffer.Simulate(m_eyeAdaptationDelayTime);

            // Update the eye adaptation shader parameters.
            m_shaderParameters.m_eyeAdaptationEnabled = m_type == ExposureControlType::EyeAdaptation;
            m_shaderParameters.m_subFrameInterpolationRatio = m_eyeAdaptationBuffer.GetSubFrameInterpolationRatio();
            m_shaderParameters.m_delaySubFrameUnitTime = m_eyeAdaptationBuffer.GetDelaySubFrameUnitTime();
            m_shaderParameters.m_needUpdateEyeAdaptationHistoryBuffer = m_eyeAdaptationBuffer.IsHistoryBufferUpdateRequired();
            m_eyeAdaptationBuffer.CalculateCurrentBufferIndices(m_shaderParameters.m_bufferIndices);

            m_buffer->UpdateData(&m_shaderParameters, sizeof(m_shaderParameters), 0);
        }

        void ExposureControlRenderProxy::UpdateViewSrg()
        {
            if (m_viewSrg == nullptr)
            {
                return;
            }

            m_eyeAdaptationBuffer.UpdateSrg();

            m_viewSrg->SetBufferView(m_exposureControlBufferInputIndex, m_buffer->GetBufferView());
            if (m_viewPtr)
            {
                m_viewPtr->InvalidateSrg();
            }
        }

        void ExposureControlRenderProxy::DeclareAttachmentsToScopeBuilder(RHI::FrameGraphScopeBuilder& scopeBuilder)
        {
            {
                RHI::BufferScopeAttachmentDescriptor desc;
                desc.m_attachmentId = GetBuffer()->GetAttachmentId();
                desc.m_bufferViewDescriptor = GetBufferViewDescriptorRead();
                desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

                scopeBuilder.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read);

                // [GFX TODO][ATOM-2739] The DX12 validation layer error occurs if a buffer is initilaized with initial data.
                // To avoid this problem, upload initial data to the RWBuffer after FrameGraphAttachmentBuilder::ImportBuffer() called.
                GetEyeAdaptationBuffer().UploadInitialDataIfNeeded();
            }
        }

        Data::Instance<RPI::ShaderResourceGroup> ExposureControlRenderProxy::GetViewSrg()
        {
            return m_viewSrg;
        }

        AZ::Data::Instance<RPI::Buffer> ExposureControlRenderProxy::GetBuffer()
        {
            return m_eyeAdaptationBuffer.GetBuffer();
        }

        const RHI::BufferViewDescriptor& ExposureControlRenderProxy::GetBufferViewDescriptorRead() const
        {
            return m_eyeAdaptationBuffer.GetBufferViewDescriptorRead();
        }

        EyeAdaptationHistoryBuffer& ExposureControlRenderProxy::GetEyeAdaptationBuffer()
        {
            return m_eyeAdaptationBuffer;
        }

        void ExposureControlRenderProxy::SetExposureControlType(ExposureControlType type)
        {
            m_type = type;
        }

        void ExposureControlRenderProxy::SetManualCompensationValue(float value)
        {
            m_shaderParameters.m_compensationValue = value;
        }

        void ExposureControlRenderProxy::SetLightAdaptationScale(float scale)
        {
            m_shaderParameters.m_adaptationScale_light_dark[static_cast<uint32_t>(EyeAdaptationType::Light)] = scale;
        }

        void ExposureControlRenderProxy::SetDarkAdaptationScale(float scale)
        {
            m_shaderParameters.m_adaptationScale_light_dark[static_cast<uint32_t>(EyeAdaptationType::Dark)] = scale;
        }

        void ExposureControlRenderProxy::SetLightAdaptationSensitivity(float sensitivity)
        {
            m_shaderParameters.m_adaptationSensitivity_light_dark[static_cast<uint32_t>(EyeAdaptationType::Light)] = sensitivity;
        }

        void ExposureControlRenderProxy::SetDarkAdaptationSensitivity(float sensitivity)
        {
            m_shaderParameters.m_adaptationSensitivity_light_dark[static_cast<uint32_t>(EyeAdaptationType::Dark)] = sensitivity;
        }

        void ExposureControlRenderProxy::SetLightAdaptationSpeedLimit(float speedLimit)
        {
            m_shaderParameters.m_adaptationSpeedLimitLog2_light_dark[static_cast<uint32_t>(EyeAdaptationType::Light)] = speedLimit;
        }

        void ExposureControlRenderProxy::SetDarkAdaptationSpeedLimit(float speedLimit)
        {
            m_shaderParameters.m_adaptationSpeedLimitLog2_light_dark[static_cast<uint32_t>(EyeAdaptationType::Dark)] = speedLimit;
        }

        void ExposureControlRenderProxy::SetEyeAdaptationExposureMin(float minExposure)
        {
            m_shaderParameters.m_exposureMinMax[0] = minExposure;
        }

        void ExposureControlRenderProxy::SetEyeAdaptationExposureMax(float maxExposure)
        {
            m_shaderParameters.m_exposureMinMax[1] = maxExposure;
        }

        void ExposureControlRenderProxy::SetEyeAdaptationLightDarkExposureBorder(float lightDarkAdaptationExposureBorder)
        {
            m_shaderParameters.m_lightDarkExposureBorderLog2 = lightDarkAdaptationExposureBorder;
        }

        void ExposureControlRenderProxy::SetEyeAdaptationDelayTime(float delayTime)
        {
            m_eyeAdaptationDelayTime = delayTime;
        }

    } // namespace Render
} // namespace AZ
