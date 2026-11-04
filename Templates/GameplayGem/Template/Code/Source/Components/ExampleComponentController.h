// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <${Name}/${SanitizedCppName}Bus.h>
#include <${Name}/${SanitizedCppName}NotificationBus.h>
#include <${Name}/${SanitizedCppName}TypeIds.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}ComponentConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(${SanitizedCppName}ComponentConfig, ${SanitizedCppName}ComponentConfigTypeId, AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}ComponentConfig, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        bool m_isEnabled{ true };
        float m_speedMultiplier{ 1.0f };
    };

    class ${SanitizedCppName}ComponentController
        : public ${SanitizedCppName}RequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_RTTI(${SanitizedCppName}ComponentController, ${SanitizedCppName}ComponentControllerTypeId);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}ComponentController, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        ${SanitizedCppName}ComponentController() = default;
        explicit ${SanitizedCppName}ComponentController(const ${SanitizedCppName}ComponentConfig& config);
        ~${SanitizedCppName}ComponentController() override = default;

        void Init();
        void Activate(AZ::EntityId entityId);
        void Deactivate();

        void SetConfiguration(const ${SanitizedCppName}ComponentConfig& config);
        const ${SanitizedCppName}ComponentConfig& GetConfiguration() const;

        // ${SanitizedCppName}RequestBus overrides
        float GetSpeedMultiplier() const override;
        void SetSpeedMultiplier(float speed) override;
        bool IsEnabled() const override;
        void SetEnabled(bool enabled) override;
        void TriggerGameplayAction() override;

        // AZ::TickBus overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        AZ::EntityId m_entityId;
        ${SanitizedCppName}ComponentConfig m_config;
    };
} // namespace ${SanitizedCppName}
