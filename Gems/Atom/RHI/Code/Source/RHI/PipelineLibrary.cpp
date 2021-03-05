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

#include <Atom/RHI/PipelineLibrary.h>

namespace AZ
{
    namespace RHI
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

        ResultCode PipelineLibrary::Init(Device& device, const PipelineLibraryData* serializedData)
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("PipelineLibrary", false, "PipelineLibrary is initialized. This operation is only permitted on an uninitialized library.");
                    return ResultCode::InvalidOperation;
                }
            }

            ResultCode resultCode = InitInternal(device, serializedData);
            if (resultCode == ResultCode::Success)
            {
                DeviceObject::Init(device);
            }
            return resultCode;
        }

        ResultCode PipelineLibrary::MergeInto(AZStd::array_view<const PipelineLibrary*> librariesToMerge)
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
    }
}
