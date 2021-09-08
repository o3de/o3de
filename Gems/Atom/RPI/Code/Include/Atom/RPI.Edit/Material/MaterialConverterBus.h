/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

            virtual bool IsEnabled() const = 0;

            //! Returns true if material property names should be included in azmaterials. This allows unlinking of dependencies for some
            //! file types to materialtype files (e.g. fbx). 
            virtual bool ShouldIncludeMaterialPropertyNames() const = 0;

            //! Converts data from a IMaterialData object to an Atom MaterialSourceData.
            //! Only works when IsEnabled() is true.
            //! @return true if the MaterialSourceData output was populated with converted material data.
            virtual bool ConvertMaterial(const AZ::SceneAPI::DataTypes::IMaterialData& materialData, MaterialSourceData& out) = 0;

            //! Returns the path to the .materialtype file that the converted materials are based on, such as StandardPBR.materialtype, etc.
            virtual AZStd::string GetMaterialTypePath() const = 0;

            //! Returns the path to a .material file to use as the default material when conversion is disabled.
            virtual AZStd::string GetDefaultMaterialPath() const = 0;
        };

        using MaterialConverterBus = AZ::EBus<MaterialConverterRequests>;

    } // namespace RPI
} // namespace AZ
