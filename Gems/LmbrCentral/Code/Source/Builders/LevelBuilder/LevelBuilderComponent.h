/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
