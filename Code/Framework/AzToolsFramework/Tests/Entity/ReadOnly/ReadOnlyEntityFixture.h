/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzTest/AzTest.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>

namespace AzToolsFramework
{
    class ReadOnlyEntityFixture
        : public UnitTest::ToolsApplicationFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        void GenerateTestHierarchy();
        AZ::EntityId CreateEditorEntity(const char* name, AZ::EntityId parentId);

        AZStd::unordered_map<AZStd::string, AZ::EntityId> m_entityMap;

        ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;

    public:
        inline static const char* RootEntityName = "Root";
        inline static const char* ChildEntityName = "Child";
        inline static const char* GrandChild1EntityName = "GrandChild1";
        inline static const char* GrandChild2EntityName = "GrandChild2";
    };

    class ReadOnlyHandlerAlwaysTrue
        : public ReadOnlyEntityQueryRequestBus::Handler
    {
    public:
        ReadOnlyHandlerAlwaysTrue();
        ~ReadOnlyHandlerAlwaysTrue();

        // ReadOnlyEntityQueryNotificationBus overrides ...
        void IsReadOnly(const AZ::EntityId& entityId, bool& isReadOnly) override;
    };

    class ReadOnlyHandlerAlwaysFalse
        : public ReadOnlyEntityQueryRequestBus::Handler
    {
    public:
        ReadOnlyHandlerAlwaysFalse();
        ~ReadOnlyHandlerAlwaysFalse();

        // ReadOnlyEntityQueryNotificationBus overrides ...
        void IsReadOnly([[maybe_unused]] const AZ::EntityId& entityId, [[maybe_unused]] bool& isReadOnly) override {}
    };

    class ReadOnlyHandlerEntityId
        : public ReadOnlyEntityQueryRequestBus::Handler
    {
    public:
        ReadOnlyHandlerEntityId(AZ::EntityId entityId);
        ~ReadOnlyHandlerEntityId();

        // ReadOnlyEntityQueryNotificationBus overrides ...
        void IsReadOnly(const AZ::EntityId& entityId, bool& isReadOnly) override;

    private:
        AZ::EntityId m_entityId;
    };
}
