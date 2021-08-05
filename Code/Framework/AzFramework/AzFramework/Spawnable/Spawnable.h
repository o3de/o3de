/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Spawnable/SpawnableMetaData.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    class Spawnable final
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(Spawnable, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::Spawnable, "{855E3021-D305-4845-B284-20C3F7FDF16B}", AZ::Data::AssetData);

        using EntityList = AZStd::vector<AZStd::unique_ptr<AZ::Entity>>;

        inline static constexpr const char* FileExtension = "spawnable";
        inline static constexpr const char* DotFileExtension = ".spawnable";

        Spawnable() = default;
        explicit Spawnable(const AZ::Data::AssetId& id, AssetStatus status = AssetStatus::NotLoaded);
        Spawnable(const Spawnable& rhs) = delete;
        Spawnable(Spawnable&& other) = delete;
        ~Spawnable() override = default;

        Spawnable& operator=(const Spawnable& rhs) = delete;
        Spawnable& operator=(Spawnable&& other) = delete;

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
