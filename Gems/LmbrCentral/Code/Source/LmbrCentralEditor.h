/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "LmbrCentral.h"

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
    {
    public:
        AZ_RTTI(LmbrCentralEditorModule, "{1BF648D7-3703-4B52-A688-67C253A059F2}", LmbrCentralModule);

        LmbrCentralEditorModule();
        ~LmbrCentralEditorModule();
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace LmbrCentral
