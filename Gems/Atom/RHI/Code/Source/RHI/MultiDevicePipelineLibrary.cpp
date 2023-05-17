/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDevicePipelineLibrary.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/PipelineLibrary.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace RHI
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

        ResultCode MultiDevicePipelineLibrary::Init(
            MultiDevice::DeviceMask deviceMask, const MultiDevicePipelineLibraryDescriptor& descriptor)
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

                    m_devicePipelineLibraries[deviceIndex] = Factory::Get().CreatePipelineLibrary();
                    resultCode =
                        m_devicePipelineLibraries[deviceIndex]->Init(*device, descriptor.GetDevicePipelineLibraryDescriptor(deviceIndex));

                    return resultCode == ResultCode::Success;
                });

            return resultCode;
        }

        ResultCode MultiDevicePipelineLibrary::MergeInto(AZStd::span<const MultiDevicePipelineLibrary* const> librariesToMerge)
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            ResultCode result = ResultCode::Success;

            for (auto& [deviceIndex, devicePipelineLibrary] : m_devicePipelineLibraries)
            {
                AZStd::vector<const PipelineLibrary*> deviceLibrariesToMerge(librariesToMerge.size());

                for (int i = 0; i < librariesToMerge.size(); ++i)
                    deviceLibrariesToMerge[i] = librariesToMerge[i]->m_devicePipelineLibraries.at(deviceIndex).get();

                result = devicePipelineLibrary->MergeInto(deviceLibrariesToMerge);

                if (result != ResultCode::Success)
                    break;
            }

            return result;
        }

        void MultiDevicePipelineLibrary::Shutdown()
        {
            if (IsInitialized())
            {
                m_devicePipelineLibraries.clear();
                MultiDeviceObject::Shutdown();
            }
        }

        bool MultiDevicePipelineLibrary::IsMergeRequired() const
        {
            bool result = false;

            for (auto& [deviceIndex, devicePipelineLibrary] : m_devicePipelineLibraries)
            {
                result |= devicePipelineLibrary->IsMergeRequired();
            }

            return result;
        }
    } // namespace RHI
} // namespace AZ
