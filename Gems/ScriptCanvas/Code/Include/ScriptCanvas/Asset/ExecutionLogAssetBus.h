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
#pragma once

#include <AzCore/EBus/EBus.h>
#include <ScriptCanvas/Asset/ExecutionLogAsset.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace ScriptCanvas
{
    class ExecutionLogAssetBus
        : public AZ::EBusTraits
    {
    public:
        virtual void ClearLog() = 0;
        
        virtual void ClearLogExecutionOverride() = 0;
        
        virtual AZ::Data::Asset<ExecutionLogAsset> LoadFromRelativePath(AZStd::string_view path) = 0;
        
        virtual void SaveToRelativePath(AZStd::string_view path) = 0;
        
        virtual void SetLogExecutionOverride(bool value) = 0;
    };
    using ExecutionLogAssetEBus = AZ::EBus<ExecutionLogAssetBus>;
}
