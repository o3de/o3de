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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Vegetation/Editor/EditorVegetationComponentTypeIds.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>
#include <CrySystemBus.h>

namespace Vegetation
{
    /*
    * The base class for all Vegetation editor classes
    */

    template<typename TComponent, typename TConfiguration>
    class EditorVegetationComponentBase
        : public LmbrCentral::EditorWrappedComponentBase<TComponent, TConfiguration>
        , private CrySystemEventBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TComponent, TConfiguration>;
        using BaseClassType::GetEntityId;

        using WrappedComponentType = TComponent;
        using WrappedConfigType = TConfiguration;

        AZ_RTTI((EditorVegetationComponentBase, "{4A00AE4F-3D10-4B9F-914A-FAA7D2579035}", TComponent, TConfiguration), BaseClassType);

        ~EditorVegetationComponentBase() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCryEditorBeginLevelExport() override;
        void OnCryEditorEndLevelExport(bool /*success*/) override;

    protected:
        using BaseClassType::m_configuration;
        using BaseClassType::m_component;
        using BaseClassType::m_visible;

        AZ::u32 ConfigurationChanged() override;
    };
} // namespace Vegetation

#include "EditorVegetationComponentBase.inl"