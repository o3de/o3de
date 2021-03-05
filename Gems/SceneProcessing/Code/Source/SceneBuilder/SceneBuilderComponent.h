/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        
    private:
        SceneBuilderWorker m_sceneBuilder;
    };
} // namespace SceneBuilder
