/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Component.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>

namespace ScriptCanvasEditor
{
    class EntityMimeDataHandler
        : public AZ::Component
        , protected GraphCanvas::SceneMimeDelegateHandlerRequestBus::Handler
    {
    public:

        AZ_COMPONENT(EntityMimeDataHandler, "{C5557609-DBB6-4ACA-A042-D03844B1EB2B}");

        static void Reflect(AZ::ReflectContext* context);

        EntityMimeDataHandler();

        // SceneMimeDelegateHandlerRequestBus
        bool IsInterestedInMimeData(const AZ::EntityId& sceneId, const QMimeData* mimeData) override;
        void HandleMove(const AZ::EntityId& sceneId, const QPointF& movePoint, const QMimeData* mimeData) override;
        void HandleDrop(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) override;
        void HandleLeave(const AZ::EntityId& sceneId, const QMimeData* mimeData) override;
        ////
        

        //AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
}
