/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiHierarchyInteractivityToggleBus.h>

#include <AzCore/Component/Component.h>
#include <LyShine/Bus/UiInitializationBus.h>

class UiHierarchyInteractivityToggleComponent
    : public AZ::Component
    , public UiInitializationBus::Handler
    , public UiHierarchyInteractivityToggleBus::Handler
{
    public:
        AZ_COMPONENT_DECL(UiHierarchyInteractivityToggleComponent);
        
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        
        //UiInitializationBus::Handler overrides..
        void InGamePostActivate() override;
        //~UiInitializationBus::Handler overrides...
        
        //UiHierarchyInteractivityToggleBus::Handler overrides..
        //! The root Method call used to manipulate the Interactive state.
        bool SetInteractivity(bool enabled) override;
        bool SetParentInteractivity(bool parentEnabled) override; //Used for child propagation.
        //! Getter to see the current interactive state.
        bool GetInteractiveState() override { return m_isInteractionLocallyEnabled && m_isInteractionParentEnabled; };
        //~UiHierarchyInteractivityToggleBus::Handler overrides...
    
        //Local Methods.
        void UpdateInteractiveState();
        void DoRecursiveSetInteractivityToChildren(const AZ::EntityId& parentId, bool parentState);

        void Activate() override;
        void Deactivate() override;

    protected:
        //State
        bool m_isInteractionLocallyEnabled = true;
        bool m_isInteractionParentEnabled = true;
};
