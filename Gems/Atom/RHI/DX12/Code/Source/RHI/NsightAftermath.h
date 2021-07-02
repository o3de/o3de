/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
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
