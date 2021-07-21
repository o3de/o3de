/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/ShaderResourceGroup.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<ShaderResourceGroup> ShaderResourceGroup::Create()
        {
            return aznew ShaderResourceGroup();
        }

        const ShaderResourceGroupCompiledData& ShaderResourceGroup::GetCompiledData() const
        {
            return m_compiledData[m_compiledDataIndex];
        }
    }
}
