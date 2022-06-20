/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <SceneBuilder/SceneBuilderWorker.h>

namespace AZ
{
    class Entity;
} // namespace AZ

namespace SceneBuilder
{
    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{47BB00DE-2C6F-4A8E-9DCF-9A226DF0D649}")
        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;
        
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        
    private:
        SceneBuilderWorker m_sceneBuilder;
    };
} // namespace SceneBuilder
