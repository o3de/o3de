/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/RTTI.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class Instance;

    class EntityIdPathMapperInterface
    {
    public:
        AZ_RTTI(EntityIdPathMapperInterface, "{1D1E032C-EEBA-449B-A265-03A8DE937EC9}");

        virtual AZ::IO::PathView GetHashedPathUsedForEntityIdGeneration(const AZ::EntityId) = 0;
        virtual void SetHashedPathUsedForEntityIdGeneration(const AZ::EntityId, AZ::IO::PathView) = 0;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
