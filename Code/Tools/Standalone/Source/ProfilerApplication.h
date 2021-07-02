/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/StandaloneToolsApplication.h>

namespace Driller
{
    class Application
        : public StandaloneTools::BaseApplication
    {
    public:
        Application(int &argc, char **argv) : BaseApplication(argc, argv) {}

    protected:
        void RegisterCoreComponents() override;
        void CreateApplicationComponents() override;
        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;
    };
}
