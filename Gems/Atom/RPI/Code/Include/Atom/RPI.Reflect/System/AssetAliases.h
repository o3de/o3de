/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>

#include <Atom/RPI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! AssetAliases stores a mapping of names to AssetIds. It allows the user to reference assets by a short name rather than a complex AssetId.
        //! For example, pass template library can use this data to map pass template name to pass template asset id. 
        class AssetAliases final
        {
            friend class AssetAliasesSourceData;
        public:
            AZ_RTTI(AssetAliases, "{0C4796E1-5229-4A09-B093-59A09A122A2F}");
            AZ_CLASS_ALLOCATOR(AssetAliases, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);
                
            Data::AssetId FindAssetId(const Name& alias) const;

            const AZStd::unordered_map<AZStd::string, Data::AssetId>& GetAssetMapping() const;
          
        private:
            AZStd::unordered_map<AZStd::string, Data::AssetId> m_assetMapping;
        };

    } // namespace RPI
} // namespace AZ
