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

        /// Given a ShaderResourceGroup and a reference ConstantsData input, this function will fetch the ConstantsData on the SRG and compare it
        /// to the reference ConstantsData. It will print the names of any constants that are different between the two.
        /// The parameter updateReferenceData can be used to set the reference data to the SRG's constant data after the comparison. This is
        /// useful for keeping track of differences in between calls to the function, such as between frames.
        void PrintConstantDataDiff(const ShaderResourceGroup& shaderResourceGroup, ConstantsData& referenceData, bool updateReferenceData = false);

        /// Given a DrawItem, an SRG binding slot on that draw item and a reference ConstantsData input, this function will fetch the ConstantsData
        /// from the draw item's SRG at the binding slot and compare it to the reference ConstantsData. It will print the names of any constants
        /// that are different between the two.
        /// The parameter updateReferenceData can be used to set the reference data to the draw item's constant data after the comparison. This is
        /// useful for keeping track of differences in between calls to the function, such as between frames.
        void PrintConstantDataDiff(const DrawItem& drawItem, ConstantsData& referenceData, uint32_t srgBindingSlot, bool updateReferenceData = false);

    }
}
