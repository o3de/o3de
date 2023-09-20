/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace AZ::SceneAPI
{
    struct SceneImportSettings
    {
    public:
        AZ_RTTI(SceneImportSettings, "{C91CB428-5081-439B-AC40-6149624832D4}");
        virtual ~SceneImportSettings() = default;

        static void Reflect(ReflectContext* context);

        bool m_optimizeScene = false;
        bool m_optimizeMeshes = false;
    };
} //namespace AZ::SceneAPI
