/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "DebugComponent.h"
#include <AzCore/Component/TickBus.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorDebugComponent
        : public EditorVegetationComponentBase<DebugComponent, DebugConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<DebugComponent, DebugConfig>;
        AZ_EDITOR_COMPONENT(EditorDebugComponent, "{BE98DFCB-6890-4E87-920B-067B2D853538}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Debugger";
        static constexpr const char* const s_componentDescription = "";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";

    protected:
        void OnDumpDataToFile();
        void OnClearReport();
        void OnRefreshAllAreas();
        void OnClearAllAreas();

    private:

        DebugRequests::PerformanceReport m_report;
    };
}
