/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SceneBuilderSystemComponent.h"
#include "ImportContexts/AssImpImportContextProvider.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContextProvider.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            void SceneBuilderSystemComponent::Activate()
            {
                // Get the import context registy
                if (auto* registry = ImportContextRegistryInterface::Get())
                {
                    // Create and register the ImportContextProvider for AssImp
                    // It should be always present and be used as a fallback if no specialized provider is available
                    auto assImpContextProvider = aznew AssImpImportContextProvider();
                    registry->RegisterContextProvider(assImpContextProvider);
                    AZ_Info("SceneAPI", "AssImp Import Context was registered.\n");
                }
                else
                {
                    AZ_Error("SceneAPI", false, "ImportContextRegistryInterface not found. AssImp Import Context was not registered.");
                }
            }

            void SceneBuilderSystemComponent::Deactivate()
            {
            }

            void SceneBuilderSystemComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SceneBuilderSystemComponent, AZ::Component>()->Version(1);
                }
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
