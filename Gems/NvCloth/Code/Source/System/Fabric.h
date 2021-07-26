/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <NvCloth/Types.h>

#include <System/NvTypes.h>

namespace NvCloth
{
    //! Fabric objects are the instances of FabricCookedData.
    //! There will be only one Fabric created per FabricCookedData,
    //! hold by SystemComponent and identified by FabricId.
    //!
    //! It has a counter of how many Cloth instances have been created using this fabric,
    //! the moment the counter is zero (when the last cloth using this fabric has been destroyed)
    //! th fabric will be automatically destroyed.
    class Fabric
    {
    public:
        Fabric(
            const FabricCookedData& cookedData,
            NvFabricUniquePtr nvFabric)
            : m_id(cookedData.m_id)
            , m_nvFabric(AZStd::move(nvFabric))
            , m_cookedData(cookedData)
        {
        }

        //! Returns the list of phase types (horizontal, vertical, bending or shearing)
        //! created for the fabric when it was cooked.
        const AZStd::vector<int32_t>& GetPhaseTypes() const
        {
            return m_cookedData.m_internalData.m_phaseTypes;
        }

        //! Fabric unique id.
        //! @note It is the same id from its FabricCookedData.
        FabricId m_id;

        //! NvCloth fabric object.
        NvFabricUniquePtr m_nvFabric;

        //! Fabric cooked data used to construct this fabric.
        FabricCookedData m_cookedData;

        //! Counter of Cloth instances created with this fabric.
        int m_numClothsUsingFabric = 0;
    };
} // namespace NvCloth
