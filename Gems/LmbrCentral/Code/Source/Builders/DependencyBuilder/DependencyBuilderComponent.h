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
#include "SeedBuilderWorker/SeedBuilderWorker.h"

namespace DependencyBuilder
{
    //! The DependencyBuilderComponent is responsible for setting up all the DependencyBuilderWorker.
    class DependencyBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(DependencyBuilderComponent, "{7748203E-5D28-474B-BC0A-74DA068D0CAE}");

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////


    private:
        SeedBuilderWorker m_seedBuilderWorker;
    };
}
