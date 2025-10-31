/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ConstantsData.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupDebug.h>

namespace AZ::RHI
{
    void PrintConstantDataDiff(const DeviceShaderResourceGroup& shaderResourceGroup, ConstantsData& referenceData, bool updateReferenceData)
    {
        const RHI::ConstantsData& currentData = shaderResourceGroup.GetData().GetConstantsData();

        AZStd::vector<RHI::ShaderInputConstantIndex> differingIndices = currentData.GetIndicesOfDifferingConstants(referenceData);

        if (differingIndices.size() > 0)
        {
            AZ_Printf("RHI", "Detected different SRG values for the following fields:\n");
            if (currentData.GetLayout())
            {
                currentData.GetLayout()->DebugPrintNames(differingIndices);
            }
        }

        if (updateReferenceData)
        {
            referenceData = currentData;
        }
    }

    void PrintConstantDataDiff(const DeviceDrawItem& drawItem, ConstantsData& referenceData, uint32_t srgBindingSlot, bool updateReferenceData)
    {
        int srgIndex = -1;
        for (uint32_t i = 0; i < drawItem.m_shaderResourceGroupCount; ++i)
        {
            if (drawItem.m_shaderResourceGroups[i]->GetBindingSlot() == srgBindingSlot)
            {
                srgIndex = i;
                break;
            }
        }

        if (srgIndex != -1)
        {
            const DeviceShaderResourceGroup& srg = *drawItem.m_shaderResourceGroups[srgIndex];
            PrintConstantDataDiff(srg, referenceData, updateReferenceData);
        }
    }
}
