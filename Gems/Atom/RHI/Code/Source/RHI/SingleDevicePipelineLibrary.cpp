/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/SingleDevicePipelineLibrary.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ::RHI
{
    bool SingleDevicePipelineLibrary::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (!IsInitialized())
            {
                AZ_Error("SingleDevicePipelineLibrary", false, "SingleDevicePipelineLibrary is not initialized. This operation is only permitted on an initialized library.");
                return false;
            }
        }
        return true;
    }

    ResultCode SingleDevicePipelineLibrary::Init(Device& device, const SingleDevicePipelineLibraryDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("SingleDevicePipelineLibrary", false, "SingleDevicePipelineLibrary is initialized. This operation is only permitted on an uninitialized library.");
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

    ResultCode SingleDevicePipelineLibrary::MergeInto(AZStd::span<const SingleDevicePipelineLibrary* const> librariesToMerge)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        return MergeIntoInternal(librariesToMerge);
    }

    void SingleDevicePipelineLibrary::Shutdown()
    {
        if (IsInitialized())
        {
            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }

    ConstPtr<PipelineLibraryData> SingleDevicePipelineLibrary::GetSerializedData() const
    {
        if (!ValidateIsInitialized())
        {
            return nullptr;
        }

        return GetSerializedDataInternal();
    }
    
    bool SingleDevicePipelineLibrary::SaveSerializedData(const AZStd::string& filePath) const
    {
        if (!ValidateIsInitialized())
        {
            return false;
        }

        return SaveSerializedDataInternal(filePath);
    }

    bool SingleDevicePipelineLibrary::IsMergeRequired() const
    {
        return true;
    }
}
