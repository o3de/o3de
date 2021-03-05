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

#include "EditorBaseShapeComponent.h"
#include "DiskShapeComponent.h"
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorDiskShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorDiskShapeComponent, EditorDiskShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorDiskShapeComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

    private:
        AZ_DISABLE_COPY_MOVE(EditorDiskShapeComponent)

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void ConfigurationChanged();

        DiskShape m_diskShape; ///< Stores underlying disk representation for this component.
    };
} // namespace LmbrCentral
