/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    namespace Components
    {
        class SelectionComponent : public EditorComponentBase
        {
        public:
            AZ_COMPONENT(SelectionComponent, "{73B724FC-43D1-4C75-ACF5-79AA8A3BF89D}", EditorComponentBase)
            SelectionComponent() = default;
            ~SelectionComponent() override = default;

        private:
            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Components
} // namespace AzToolsFramework
