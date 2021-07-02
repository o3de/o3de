/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            void BehaviorComponent::Activate()
            {
            }

            void BehaviorComponent::Deactivate()
            {
            }

            void BehaviorComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<BehaviorComponent, AZ::Component>()->Version(1);
                }
            }
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ
