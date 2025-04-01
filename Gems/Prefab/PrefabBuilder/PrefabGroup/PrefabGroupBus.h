
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <PrefabGroup/PrefabGroup.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <AzCore/Component/EntityId.h>

namespace AZ::SceneAPI::DataTypes
{
    class ICustomPropertyData;
}

namespace AZ::SceneAPI::SceneData
{
    class PrefabGroup;
}

namespace AZ::SceneAPI::Containers
{
    class Scene;
}

namespace AZ::SceneAPI
{
    using PrefabGroup = AZ::SceneAPI::SceneData::PrefabGroup;
    using Scene = AZ::SceneAPI::Containers::Scene;

    //! Events that handle Prefab Group logic.
    //! The behavior context will reflect this EBus so that it can be used in Python and C++ code.
    class PrefabGroupRequests
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(PrefabGroupRequests, "{2AF2819A-59DA-4469-863A-E90D0AEF1646}");

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        PrefabGroupRequests() = default;
        virtual ~PrefabGroupRequests() = default;

        using ManifestUpdates = AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>;

        virtual AZStd::optional<ManifestUpdates> GeneratePrefabGroupManifestUpdates(const Scene& scene) const = 0;
        virtual AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>> GenerateDefaultPrefabMeshGroups(const Scene& scene) const = 0;
    };
    using PrefabGroupEventBus = AZ::EBus<PrefabGroupRequests>;

    //! Notifications during the default Prefab Group construction so that other scene builders can contribute the entity-component prefab.
    //! The behavior context will reflect this EBus so that it can be used in Python and C++ code.
    class PrefabGroupNotifications
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(PrefabGroupNotifications, "{BD88ADC3-B72F-43DD-B279-A44E39CD612F}");

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        PrefabGroupNotifications() = default;
        virtual ~PrefabGroupNotifications() = default;

        //! This is sent when the Prefab Group logic is finished creating an entity but allows other scene builders to extend the entity
        virtual void OnUpdatePrefabEntity(const AZ::EntityId& prefabEntity) = 0;
    };
    using PrefabGroupNotificationBus = AZ::EBus<PrefabGroupNotifications>;
}
