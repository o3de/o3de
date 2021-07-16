/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Module/Module.h>

namespace GraphCanvas
{
    /*!
    * The GraphCanvas module class coordinates with the application
    * to reflect classes and create system components.
    *
    */
    class GraphCanvasModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(GraphCanvasModule, "{83DD6BEA-CFA0-4848-A8EF-C4E0714B876D}", AZ::Module);

        GraphCanvasModule();
        ~GraphCanvasModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
