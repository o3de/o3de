/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiNavigationBus.h>
#include <LyShine/UiBase.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
class UiNavigationSettings
    : public UiNavigationBus::Handler
{
public: // types

    using GetNavigableEntitiesFn = AZStd::function<LyShine::EntityArray(AZ::EntityId)>;

public: // member functions
    AZ_CLASS_ALLOCATOR(UiNavigationSettings, AZ::SystemAllocator, 0);
    AZ_RTTI(UiNavigationSettings, "{E28DDC8B-F7C6-406F-966C-2F0825471641}");

    UiNavigationSettings();
    ~UiNavigationSettings();

    // UiNavigationInterface
    NavigationMode GetNavigationMode() override;
    void SetNavigationMode(NavigationMode navigationMode) override;
    AZ::EntityId GetOnUpEntity() override;
    void SetOnUpEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetOnDownEntity() override;
    void SetOnDownEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetOnLeftEntity() override;
    void SetOnLeftEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetOnRightEntity() override;
    void SetOnRightEntity(AZ::EntityId entityId) override;
    // ~UiNavigationInterface

    // Connects to the bus and stores the entity and callback function to get the navigable entities
    void Activate(AZ::EntityId entityId, GetNavigableEntitiesFn getNavigableFn);

    // Disconnects from the bus
    void Deactivate();

public: // static member functions

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

protected: // persistent data members

    //! Determines how the next element to get focus is chosen on a navigation event
    NavigationMode m_navigationMode;

    //! Entities to receive focus when a navigation event occurs
    AZ::EntityId m_onUpEntity;
    AZ::EntityId m_onDownEntity;
    AZ::EntityId m_onLeftEntity;
    AZ::EntityId m_onRightEntity;

protected: // non-persistent data members

    AZ::EntityId m_entityId;
    GetNavigableEntitiesFn m_getNavigableEntitiesFunction;

private: // types

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;

private: // member functions

    //! Methods used for controlling the Edit Context (the properties pane)
    EntityComboBoxVec PopulateNavigableEntityList();
    bool IsNavigationModeCustom() const;
};

