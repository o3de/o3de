/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
}
