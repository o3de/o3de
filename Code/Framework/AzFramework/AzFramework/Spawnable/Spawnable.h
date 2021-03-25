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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Spawnable/SpawnableMetaData.h>

namespace AzFramework
{
    class ReflectContext;

    class Spawnable final
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(Spawnable, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::Spawnable, "{855E3021-D305-4845-B284-20C3F7FDF16B}", AZ::Data::AssetData);

        using EntityList = AZStd::vector<AZStd::unique_ptr<AZ::Entity>>;

        inline static constexpr const char* FileExtension = "spawnable";

        Spawnable() = default;
        explicit Spawnable(const AZ::Data::AssetId& id);
        Spawnable(const Spawnable& rhs) = delete;
        Spawnable(Spawnable&& other);
        ~Spawnable() override = default;

        Spawnable& operator=(const Spawnable& rhs) = delete;
        Spawnable& operator=(Spawnable&& other);

        const EntityList& GetEntities() const;
        EntityList& GetEntities();
        bool IsEmpty() const;

        SpawnableMetaData& GetMetaData();
        const SpawnableMetaData& GetMetaData() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        SpawnableMetaData m_metaData;

        // Container for keeping all entities of the prefab the Spawnable was created from.
        // Includes both direct and nested entities of the prefab.
        EntityList m_entities;
    };

    using SpawnableList = AZStd::vector<Spawnable>;

} // namespace AzFramework
