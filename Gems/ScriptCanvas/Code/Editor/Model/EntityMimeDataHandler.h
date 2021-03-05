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