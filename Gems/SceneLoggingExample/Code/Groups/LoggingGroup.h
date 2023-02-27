/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>

namespace AZ
{
    class ReflectContext;
}

namespace SceneLoggingExample
{
    // The LoggingGroup class contains the settings that will be interpreted by the exporter.
    // Groups typically contain settings and information only. They rarely implement any advanced logic.
    //
    // Groups tend to have a one-to-one relationship to the target format. For example, every mesh group
    // will produce a .cgf file in the cache. Groups also aim to be the most basic form of the required data, 
    // providing the minimum information that is needed to create a valid product in the cache. 
    // 
    // To further fine tune the group, you can add rules (also called modifiers). For example, you can add a rule to 
    // control the world matrix.
    class LoggingGroup : public AZ::SceneAPI::DataTypes::IGroup
    {
    public:
        LoggingGroup();
        ~LoggingGroup() override = default;

        AZ_RTTI(LoggingGroup, "{A5ECF95D-2E84-4574-BF93-09E469E2BA4E}", AZ::SceneAPI::DataTypes::IGroup);
        AZ_CLASS_ALLOCATOR(LoggingGroup, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        void SetName(const AZStd::string& name);
        void SetName(AZStd::string&& name);
        const AZStd::string& GetName() const override;
        const AZ::Uuid& GetId() const override;

        AZ::SceneAPI::Containers::RuleContainer& GetRuleContainer() override;
        const AZ::SceneAPI::Containers::RuleContainer& GetRuleContainerConst() const override;

        const AZStd::string& GetGraphLogRoot() const;
        bool DoesLogGraph() const;
        
        bool DoesLogProcessingEvents() const;
        void ShouldLogProcessingEvents(bool state);

    protected:
        AZ::SceneAPI::Containers::RuleContainer m_ruleContainer;
        AZStd::string m_groupName;
        AZStd::string m_graphLogRoot;
        AZ::Uuid m_id;
        bool m_logProcessingEvents;

        static const char* s_disabledOption;
    };
} // namespace SceneLoggingExample
