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

#include <Components/Connections/ConnectionVisualComponent.h>

namespace GraphCanvas
{
    class DataConnectionVisualComponent
        : public ConnectionVisualComponent
        , public ConnectionNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(DataConnectionVisualComponent, "{5016737C-59C4-4EE9-9B06-74E220833F03}", ConnectionVisualComponent);
        static void Reflect(AZ::ReflectContext* context);

        DataConnectionVisualComponent() = default;
        ~DataConnectionVisualComponent() = default;
        
    protected:
        void CreateConnectionVisual() override;

    private:
        DataConnectionVisualComponent(const DataConnectionVisualComponent &) = delete;
    };
}