/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>

#include "LevelBuilderWorker.h"

namespace LevelBuilder
{
    //! The LevelBuilderComponent is responsible for setting up the LevelBuilderWorker.
    class LevelBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(LevelBuilderComponent, "{2E2A53CB-055A-48B0-AFC4-C3C3DB82AC4D}");

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////


    private:
        LevelBuilderWorker m_levelBuilder;
    };
}
