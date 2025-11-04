/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DevicePipelineLibrary.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ::RHI
{
    bool DevicePipelineLibrary::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (!IsInitialized())
            {
                AZ_Error("DevicePipelineLibrary", false, "DevicePipelineLibrary is not initialized. This operation is only permitted on an initialized library.");
                return false;
            }
        }
        return true;
    }

    ResultCode DevicePipelineLibrary::Init(Device& device, const DevicePipelineLibraryDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("DevicePipelineLibrary", false, "DevicePipelineLibrary is initialized. This operation is only permitted on an uninitialized library.");
                return ResultCode::InvalidOperation;
            }
        }

        ResultCode resultCode = InitInternal(device, descriptor);
        if (resultCode == ResultCode::Success)
        {
            AZStd::string libName;
            AzFramework::StringFunc::Path::GetFileName(descriptor.m_filePath.c_str(), libName);
            SetName(Name(libName));
            DeviceObject::Init(device);
        }
        return resultCode;
    }

    ResultCode DevicePipelineLibrary::MergeInto(AZStd::span<const DevicePipelineLibrary* const> librariesToMerge)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        return MergeIntoInternal(librariesToMerge);
    }

    void DevicePipelineLibrary::Shutdown()
    {
        if (IsInitialized())
        {
            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }

    ConstPtr<PipelineLibraryData> DevicePipelineLibrary::GetSerializedData() const
    {
        if (!ValidateIsInitialized())
        {
            return nullptr;
        }

        return GetSerializedDataInternal();
    }
    
    bool DevicePipelineLibrary::SaveSerializedData(const AZStd::string& filePath) const
    {
        if (!ValidateIsInitialized())
        {
            return false;
        }

        return SaveSerializedDataInternal(filePath);
    }

    bool DevicePipelineLibrary::IsMergeRequired() const
    {
        return true;
    }
}
