/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace RPI
    {

        //! This class is used in data driven contexts to point to another asset using a file path.
        //! The asset builders for classes that uses this can look up the asset ID of the file path,
        //! add it to it's list of dependencies, and then set the m_assetId for runtime use by the class.
        //! For an example of this, see PassAssetBuilder
        struct ATOM_RPI_REFLECT_API AssetReference final
        {
            AZ_RTTI(AssetReference, "{33895678-2406-46F5-9303-103C3FB6C40F}");
            AZ_CLASS_ALLOCATOR(AssetReference, SystemAllocator);
            static void Reflect(ReflectContext* context);

            AssetReference() = default;
            ~AssetReference() = default;

            //! File path to the source asset
            AZStd::string m_filePath;

            //! ID of the asset
            Data::AssetId m_assetId;
        };

    }
}
