/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>

#include <gmock/gmock.h>

namespace UnitTest
{
    class MockEditorVisibleEntityDataCacheInterface : public AzToolsFramework::EditorVisibleEntityDataCacheInterface
    {
        using ComponentEntityAccentType = AzToolsFramework::Components::EditorSelectionAccentSystemComponent::ComponentEntityAccentType;

    public:
        virtual ~MockEditorVisibleEntityDataCacheInterface() = default;

        // AzToolsFramework::EditorVisibleEntityDataCacheInterface overrides ...
        MOCK_CONST_METHOD0(VisibleEntityDataCount, size_t());
        MOCK_CONST_METHOD1(GetVisibleEntityPosition, AZ::Vector3(size_t));
        MOCK_CONST_METHOD1(GetVisibleEntityTransform, const AZ::Transform&(size_t));
        MOCK_CONST_METHOD1(GetVisibleEntityId, AZ::EntityId(size_t));
        MOCK_CONST_METHOD1(GetVisibleEntityAccent, ComponentEntityAccentType(size_t));
        MOCK_CONST_METHOD1(IsVisibleEntityLocked, bool(size_t));
        MOCK_CONST_METHOD1(IsVisibleEntityVisible, bool(size_t));
        MOCK_CONST_METHOD1(IsVisibleEntitySelected, bool(size_t));
        MOCK_CONST_METHOD1(IsVisibleEntityIconHidden, bool(size_t));
        MOCK_CONST_METHOD1(IsVisibleEntityIndividuallySelectableInViewport, bool(size_t));
        MOCK_CONST_METHOD1(IsVisibleEntityInFocusSubTree, bool(size_t));
        MOCK_CONST_METHOD1(GetVisibleEntityIndexFromId, AZStd::optional<size_t>(AZ::EntityId entityId));
    };
} // namespace UnitTest
