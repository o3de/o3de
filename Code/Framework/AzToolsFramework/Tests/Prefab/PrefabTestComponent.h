/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    };
}
