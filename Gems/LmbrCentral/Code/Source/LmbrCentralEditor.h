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

#include "LmbrCentral.h"

#include <LmbrCentral/Rendering/EditorMeshBus.h>

namespace Water
{
    class WaterVolumeConverter;
}

namespace LmbrCentral
{
    /**
     * The LmbrCentralEditor module class extends the \ref LmbrCentralModule
     * by defining editor versions of many components.
     *
     * This is the module used when working in the Editor.
     * The \ref LmbrCentralModule is used by the standalone game.
     */
    class LmbrCentralEditorModule
        : public LmbrCentralModule
        , public EditorMeshBus::Handler
    {
    public:
        AZ_RTTI(LmbrCentralEditorModule, "{1BF648D7-3703-4B52-A688-67C253A059F2}", LmbrCentralModule);

        LmbrCentralEditorModule();
        ~LmbrCentralEditorModule();
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

        bool AddMeshComponentWithAssetId(const AZ::EntityId& targetEntity, const AZ::Uuid& meshAssetId) override;
    };
} // namespace LmbrCentral