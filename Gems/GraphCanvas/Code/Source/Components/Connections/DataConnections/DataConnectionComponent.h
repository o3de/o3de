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

#include <Source/Components/Connections/ConnectionComponent.h>

namespace GraphCanvas
{
    class DataConnectionComponent
        : public ConnectionComponent
        , public ConnectionNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(DataConnectionComponent, "{ECC6A4D9-E8CD-451B-93BE-409F04A9A52B}", ConnectionComponent);
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateDataConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& substyle = "");

        DataConnectionComponent() = default;
        DataConnectionComponent(const Endpoint& sourceEndpoint,const Endpoint& targetEndpoint, bool createModelConnection = true);
        ~DataConnectionComponent() override = default;

        // ConnectionComponent
        bool AllowNodeCreation() const override;
        ////

    protected:

        DataConnectionComponent(const DataConnectionComponent&) = delete;
        const DataConnectionComponent& operator=(const DataConnectionComponent&) = delete;
        ConnectionMoveResult OnConnectionMoveComplete(const QPointF& scenePos, const QPoint& screenPos) override;
    }; 
}