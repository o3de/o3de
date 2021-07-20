/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
