/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/EBus/Event.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>

namespace RecastNavigation
{
    class EditorRecastNavigationMeshConfig final
    {
    public:
        AZ_RTTI(EditorRecastNavigationMeshConfig, "{5b36cc78-3434-44e5-bfab-19a86d0869cd}");

        static void Reflect(AZ::ReflectContext* context);

        bool m_showNavigationMesh = true;
        int m_updateEveryNSeconds = -1;
        int m_backgroundThreadsToUse = 4;

        void BindUpdateEveryNSecondsFieldEventHandler(AZ::Event<int>::Handler& handler);

    private:
        AZ::Crc32 OnUpdateEveryNSecondsChanged();
        AZ::Event<int> m_updateEveryNSecondsFieldEvent;
    };

} // namespace RecastNavigation
