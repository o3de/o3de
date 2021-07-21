/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Editor/MeshNodeHandler.h>

namespace NvCloth
{
    namespace Editor
    {
        AZStd::vector<AzToolsFramework::PropertyHandlerBase*> RegisterPropertyTypes()
        {
            AZStd::vector<AzToolsFramework::PropertyHandlerBase*> propertyHandlers =
            {
                aznew MeshNodeHandler()
            };

            for (const auto& handler : propertyHandlers)
            {
                AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, handler);
            }
            return propertyHandlers;
        }

        void UnregisterPropertyTypes(AZStd::vector<AzToolsFramework::PropertyHandlerBase*>& handlers)
        {
            for (auto handler : handlers)
            {
                if (!handler->AutoDelete())
                {
                    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, handler);
                    delete handler;
                }
            }
            handlers.clear();
        }
    } // namespace Editor
} // namespace NvCloth
