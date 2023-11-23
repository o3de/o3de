/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDevicePipelineLibrary.h>
#include <Atom/RHI/SingleDevicePipelineLibrary.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ::RHI
{
    bool MultiDevicePipelineLibrary::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (!IsInitialized())
            {
                AZ_Error(
                    "MultiDevicePipelineLibrary",
                    false,
                    "MultiDevicePipelineLibrary is not initialized. This operation is only permitted on an initialized library.");
                return false;
            }
        }
        return true;
    }

    ResultCode MultiDevicePipelineLibrary::Init(MultiDevice::DeviceMask deviceMask, const MultiDevicePipelineLibraryDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error(
                    "MultiDevicePipelineLibrary",
                    false,
                    "MultiDevicePipelineLibrary is initialized. This operation is only permitted on an uninitialized library.");
                return ResultCode::InvalidOperation;
            }
        }

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode = ResultCode::Success;

        IterateDevices(
            [this, &descriptor, &resultCode](int deviceIndex)
            {
                auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                m_deviceObjects[deviceIndex] = Factory::Get().CreatePipelineLibrary();
                resultCode = GetDevicePipelineLibrary(deviceIndex)->Init(*device, descriptor.GetDevicePipelineLibraryDescriptor(deviceIndex));

                return resultCode == ResultCode::Success;
            });

        if(resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific PipelineLibraries and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }


        return resultCode;
    }

    ResultCode MultiDevicePipelineLibrary::MergeInto(AZStd::span<const MultiDevicePipelineLibrary* const> librariesToMerge)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        return IterateObjects<SingleDevicePipelineLibrary>([&](auto deviceIndex, auto devicePipelineLibrary)
        {
            AZStd::vector<const SingleDevicePipelineLibrary*> deviceLibrariesToMerge;

            for (int i = 0; i < librariesToMerge.size(); ++i)
            {
                auto it = librariesToMerge[i]->m_deviceObjects.find(deviceIndex);

                if (it != librariesToMerge[i]->m_deviceObjects.end())
                {
                    deviceLibrariesToMerge.emplace_back(static_cast<const SingleDevicePipelineLibrary*>(it->second.get()));
                }
            }

            if (!deviceLibrariesToMerge.empty())
            {
                return devicePipelineLibrary->MergeInto(deviceLibrariesToMerge);
            }

            return ResultCode::Success;
        });
    }

    void MultiDevicePipelineLibrary::Shutdown()
    {
        if (IsInitialized())
        {
            m_deviceObjects.clear();
            MultiDeviceObject::Shutdown();
        }
    }

    bool MultiDevicePipelineLibrary::IsMergeRequired() const
    {
        bool result = false;

        IterateObjects<SingleDevicePipelineLibrary>([&result]([[maybe_unused]]auto deviceIndex, auto devicePipelineLibrary)
        {
            result |= devicePipelineLibrary->IsMergeRequired();
        });

        return result;
    }
} // namespace AZ::RHI
