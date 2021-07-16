/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            void RCExportingComponent::Activate()
            {
                ActivateBindings();
            }

            void RCExportingComponent::Deactivate()
            {
                DeactivateBindings();
            }

            void RCExportingComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<RCExportingComponent, AZ::Component>()->Version(2);
                }
            }
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ
