/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-debugger";

    protected:
        void OnDumpDataToFile();
        void OnClearReport();
        void OnRefreshAllAreas();
        void OnClearAllAreas();

    private:

        DebugRequests::PerformanceReport m_report;
    };
}
