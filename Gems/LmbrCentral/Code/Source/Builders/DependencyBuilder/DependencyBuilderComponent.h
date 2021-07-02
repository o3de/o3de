/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
