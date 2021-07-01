/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Uuid.h>

#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/PersistentIdBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>

namespace GraphCanvas
{
    // PersistentIdComponent provides an Id for GraphCanvas objects that will persist across loads/unloads.
    // This enables the ability to not serialize out the entire GraphCanvas object, and instead serialize out only
    // the user configurable information. But still enable things like NodeGroups to maintain some references to 
    // specific GraphCanvas objects in order to maintain their state correctly.
    class PersistentIdComponent
        : public AZ::Component
        , public PersistentIdRequestBus::Handler
        , public PersistentMemberRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(PersistentIdComponent, "{57D546EE-C074-432E-A802-77CFC2E37AE7}", AZ::Component);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        PersistentIdComponent();
        ~PersistentIdComponent() = default;
        
        // Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////
        
        // SceneMemberNotifications
        void OnSceneSet(const AZ::EntityId& graphId) override;

        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphSerialization& serializationTarget) override;
        ////
        
        // PersistentIdRequestBus
        AZ::EntityId MapToEntityId() const override;
        ////
        
        // PersistentMemberRequestBus
        PersistentGraphMemberId GetPreviousGraphMemberId() const override;
        PersistentGraphMemberId GetPersistentGraphMemberId() const override;
        ////
        
    private:
        PersistentGraphMemberId       m_previousId;
        PersistentIdComponentSaveData m_saveData;
    };
}
