/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            void ExportingComponent::Activate()
            {
                ActivateBindings();
            }

            void ExportingComponent::Deactivate()
            {
                DeactivateBindings();
            }

            void ExportingComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ExportingComponent, AZ::Component>()->Version(2);
                }
            }
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ
