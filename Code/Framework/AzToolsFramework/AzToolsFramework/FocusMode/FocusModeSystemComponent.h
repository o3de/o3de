/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/FocusMode/FocusModeInterface.h>

namespace AzToolsFramework
{
    bool IsInFocusSubTree(AZ::EntityId entityId, AZ::EntityId focusRootId);

    //! System Component to handle the Editor Focus Mode system
    class FocusModeSystemComponent final
        : public AZ::Component
        , private FocusModeInterface
    {
    public:
        AZ_COMPONENT(FocusModeSystemComponent, "{6CE522FE-2057-4794-BD05-61E04BD8EA30}");

        FocusModeSystemComponent() = default;
        virtual ~FocusModeSystemComponent() = default;

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // FocusModeInterface overrides ...
        void SetFocusRoot(AZ::EntityId entityId) override;
        void ClearFocusRoot(AzFramework::EntityContextId entityContextId) override;
        AZ::EntityId GetFocusRoot(AzFramework::EntityContextId entityContextId) override;
        bool IsInFocusSubTree(AZ::EntityId entityId) const override;

    private:
        AZ::EntityId m_focusRoot;
    };

} // namespace AzToolsFramework
