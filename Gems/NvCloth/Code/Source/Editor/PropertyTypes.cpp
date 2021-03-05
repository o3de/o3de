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
