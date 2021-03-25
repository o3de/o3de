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
* This software contains source code provided by NVIDIA Corporation.
*/

#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/std/string/string.h>
#include <RHI/DX12.h>

namespace Aftermath
{
    void SetAftermathEventMarker(void* cntxHandle, const AZStd::string& markerData, bool isAftermathInitialized);
    bool InitializeAftermath(AZ::RHI::Ptr<ID3D12DeviceX> dx12Device);
    void* CreateAftermathContextHandle(ID3D12GraphicsCommandList* commandList, void* crashTracker);
    void OutputLastScopeExecutingOnGPU(void* crashTracker);
}
