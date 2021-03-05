/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/Component.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>

namespace TouchBending
{
    namespace Pipeline
    {
        /*!
          This class serves two purposes, the first one is to declare dependency on SceneProcessingConfigService, this
          guarantees that SceneProcessingConfigService is activated before this component.
          Given the previous guarantee, this component can safely register a new virtualtype for TouchBendable
          geometry within FBX files.
          The second purpose is to expose the virtualtype in the editor context. By reflecting
          the virtualtype in the editor context, the user can further customize the naming pattern for touchbendable
          geometry.
        */
        class TouchBendingSceneSystemComponent
            : public AZ::SceneAPI::SceneCore::SceneSystemComponent
        {
        public:
            AZ_COMPONENT(TouchBendingSceneSystemComponent, "{42819C07-7EF5-47F1-B7A5-65BDDD34C1CC}", AZ::SceneAPI::SceneCore::SceneSystemComponent);

            TouchBendingSceneSystemComponent();
            ~TouchBendingSceneSystemComponent() override = default;

            void Activate() override;
            void Deactivate() override;

            static void Reflect(AZ::ReflectContext* context);

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        private:
            AZ::SceneAPI::SceneCore::PatternMatcher m_patternMatcher;
            AZStd::string                           m_virtualType;
            bool                                    m_includeChildren;
        };
    } // Pipeline
} // TouchBending