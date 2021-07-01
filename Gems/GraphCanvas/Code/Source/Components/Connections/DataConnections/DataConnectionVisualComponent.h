/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
