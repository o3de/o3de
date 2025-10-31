/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AZ::IO::Path PrefabLoaderInterface::GeneratePath()
        {
            return AZStd::string::format("Prefab_%s", AZ::Entity::MakeId().ToString().c_str());
        }

    } // namespace Prefab
} // namespace AzToolsFramework
