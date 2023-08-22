/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/PipelineLibrary.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ::RHI
{
    bool PipelineLibrary::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (!IsInitialized())
            {
                AZ_Error("PipelineLibrary", false, "PipelineLibrary is not initialized. This operation is only permitted on an initialized library.");
                return false;
            }
        }
        return true;
    }

    ResultCode PipelineLibrary::Init(Device& device, const PipelineLibraryDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("PipelineLibrary", false, "PipelineLibrary is initialized. This operation is only permitted on an uninitialized library.");
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

    ResultCode PipelineLibrary::MergeInto(AZStd::span<const PipelineLibrary* const> librariesToMerge)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        return MergeIntoInternal(librariesToMerge);
    }

    void PipelineLibrary::Shutdown()
    {
        if (IsInitialized())
        {
            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }

    ConstPtr<PipelineLibraryData> PipelineLibrary::GetSerializedData() const
    {
        if (!ValidateIsInitialized())
        {
            return nullptr;
        }

        return GetSerializedDataInternal();
    }
    
    bool PipelineLibrary::SaveSerializedData(const AZStd::string& filePath) const
    {
        if (!ValidateIsInitialized())
        {
            return false;
        }

        return SaveSerializedDataInternal(filePath);
    }

    bool PipelineLibrary::IsMergeRequired() const
    {
        return true;
    }
}
