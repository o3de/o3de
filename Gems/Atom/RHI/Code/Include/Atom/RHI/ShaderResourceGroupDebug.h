/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    namespace RHI
    {
        class ConstantsData;
        struct DrawItem;
        class ShaderResourceGroup;

        void PrintConstantDataDiff(const ShaderResourceGroup& shaderResourceGroup, ConstantsData& referenceData, bool updateReferenceData = false);
        void PrintConstantDataDiff(const DrawItem& drawItem, ConstantsData& referenceData, u32 srgBindingSlot, bool updateReferenceData = false);

    }
}
