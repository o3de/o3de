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
        // @deprecated - SelectionComponent has been deprecated and is no longer instantiated.
        // This type is being retained to handle legacy data serialization.
        class SelectionComponent : public EditorComponentBase
        {
        public:
            AZ_COMPONENT(SelectionComponent, "{A7CBE7BC-9B4A-47DC-962F-1BFAE85DBF3A}", EditorComponentBase)
            SelectionComponent() = default;
            ~SelectionComponent() override = default;

        private:
            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Components
} // namespace AzToolsFramework
