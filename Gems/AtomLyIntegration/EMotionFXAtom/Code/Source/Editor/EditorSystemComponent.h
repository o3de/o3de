/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <Integration/AnimationBus.h>

namespace AZ::EMotionFXAtom
{
    class EditorSystemComponent
        : public Component
        , private EMotionFX::Integration::SystemNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{1FAEC046-255D-4664-8F12-D16503C34431}");

        static void Reflect(ReflectContext* context);

    protected:
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;

        // SystemNotificationBus::OnRegisterPlugin
        void OnRegisterPlugin() override;
    };
} // namespace AZ::EMotionFXAtom
