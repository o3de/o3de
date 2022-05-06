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
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>

namespace AzToolsFramework
{
    //! System Component to handle the Action Manager system
    class ActionManagerSystemComponent final : public AZ::Component
    {
    public:
        AZ_COMPONENT(ActionManagerSystemComponent, "{47925132-7373-42EE-9131-F405EE4B0F1A}");

        ActionManagerSystemComponent() = default;
        virtual ~ActionManagerSystemComponent() = default;

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        AZStd::unique_ptr<ActionManager> m_actionManager = nullptr;
    };

} // namespace AzToolsFramework
