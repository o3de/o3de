/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AZ::SceneAPI::DataTypes
{
    class IPrefabGroup
        : public ISceneNodeGroup
    {
    public:
        AZ_RTTI(IPrefabGroup, "{7E50FAEF-3379-4521-99C5-B428FDEE3B7B}", ISceneNodeGroup);

        ~IPrefabGroup() override = default;
        virtual AzToolsFramework::Prefab::PrefabDomConstReference GetPrefabDomRef() const = 0;
    };
}
