/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<Device> Device::Create()
        {
            return aznew Device();
        }

        Device::Device()
        {
            m_descriptor.m_platformLimitsDescriptor = aznew RHI::PlatformLimitsDescriptor;
        }

        void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
        {
            formatsCapabilities.fill(static_cast<RHI::FormatCapabilities>(~0));
        }

        RHI::ResultCode Device::InitializeLimits()
        {
            constexpr uint32_t maxValue = static_cast<uint32_t>(-1);

            m_limits.m_maxImageDimension1D = maxValue;
            m_limits.m_maxImageDimension2D = maxValue;
            m_limits.m_maxImageDimension3D = maxValue;
            m_limits.m_maxImageDimensionCube = maxValue;
            m_limits.m_maxImageArraySize = maxValue;
            m_limits.m_minConstantBufferViewOffset = RHI::Alignment::Constant;
            m_limits.m_maxIndirectDrawCount = maxValue;
            m_limits.m_maxIndirectDispatchCount = maxValue;
            m_limits.m_maxConstantBufferSize = maxValue;
            m_limits.m_maxBufferSize = maxValue;

            return RHI::ResultCode::Success;
        }

        void Device::ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction)
        {
            notifyFunction();
        }

        RHI::ShadingRateImageValue Device::ConvertShadingRate([[maybe_unused]] RHI::ShadingRate rate) const
        {
            return RHI::ShadingRateImageValue{};
        }
    }
}
