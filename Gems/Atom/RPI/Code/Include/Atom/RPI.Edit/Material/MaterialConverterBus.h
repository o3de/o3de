/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMaterialData;
        }
    }
}

namespace AZ
{
    namespace RPI
    {
        //! Providers a user callback to convert from SceneAPI data into Atom materials.
        class MaterialConverterRequests
            : public AZ::EBusTraits
        {
        public:
            //! Returns true if the converion was successful
            virtual bool ConvertMaterial(const AZ::SceneAPI::DataTypes::IMaterialData& materialData, MaterialSourceData& out) = 0;
            //! Returns the path to the .materialtype file that the materials are based on, such as StandardPBR.materialtype, etc.
            virtual const char* GetMaterialTypePath() const = 0;
        };

        using MaterialConverterBus = AZ::EBus<MaterialConverterRequests>;

    } // namespace RPI
} // namespace AZ
