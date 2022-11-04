/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

 namespace AzToolsFramework
{
    namespace Prefab
    {
        bool PrefabOverrideHandler::AreOverridesPresent(AZ::Dom::Path path, LinkId linkId)
        {
            PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (prefabSystemComponentInterface != nullptr)
            {
                LinkReference link = prefabSystemComponentInterface->FindLink(linkId);
                if (link.has_value())
                {
                    return link->get().AreOverridesPresent(path);
                }
            }
            return false;
        }
    }
}
