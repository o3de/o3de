/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>

#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Configuration.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! AssetAliases stores a mapping of names to AssetIds. It allows the user to reference assets by a short name rather than a complex AssetId.
        //! For example, pass template library can use this data to map pass template name to pass template asset id. 
        class ATOM_RPI_REFLECT_API AssetAliases final
        {
            friend class AssetAliasesSourceData;
        public:
            AZ_RTTI(AssetAliases, "{0C4796E1-5229-4A09-B093-59A09A122A2F}");
            AZ_CLASS_ALLOCATOR(AssetAliases, SystemAllocator);

            static void Reflect(ReflectContext* context);
                
            Data::AssetId FindAssetId(const Name& alias) const;

            const AZStd::unordered_map<AZStd::string, Data::AssetId>& GetAssetMapping() const;
          
        private:
            AZStd::unordered_map<AZStd::string, Data::AssetId> m_assetMapping;
        };

    } // namespace RPI
} // namespace AZ
