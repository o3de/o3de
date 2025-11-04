/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace UnitTest
{
    class PrefabTestComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(PrefabTestComponent, "{C5FCF40A-FAEC-473C-BFAF-68A66DC45B33}");

        PrefabTestComponent() = default;
        explicit PrefabTestComponent(bool boolProperty);

        static void Reflect(AZ::ReflectContext* reflection);

        bool m_boolProperty = false;
        int  m_intProperty = 0;
        AZ::EntityId m_entityIdProperty;
    };

    class UnReflectedType
    {
    public:
        AZ_TYPE_INFO(UnReflectedType, "{FB65262C-CE9A-45CA-99EB-4DDCB19B32DB}");
        int m_unReflectedInt = 42;
    };

    class PrefabTestComponentWithUnReflectedTypeMember
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(PrefabTestComponentWithUnReflectedTypeMember, "{726281E1-8E47-46AB-8018-D3F4BA823D74}");
        static void Reflect(AZ::ReflectContext* reflection);

        UnReflectedType m_unReflectedType;
        int             m_reflectedType = 52;
    };

    class PrefabNonEditorComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(PrefabNonEditorComponent, "{47475C6F-3E69-493F-9EDA-B16E672BEF25}");

        PrefabNonEditorComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        void Activate() override;
        void Deactivate() override;

        int m_intProperty = 0;
    };
    
}
