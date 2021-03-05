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

#include <Config/SettingsObjects/SoftNameSetting.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        class NodeSoftNameSetting : public SoftNameSetting
        {
        public:
            AZ_CLASS_ALLOCATOR(NodeSoftNameSetting, SystemAllocator, 0);
            AZ_RTTI(NodeSoftNameSetting, "{74629DAE-641A-4BCE-B6D5-3F7DD9F647FA}", SoftNameSetting);

            NodeSoftNameSetting() = default;
            NodeSoftNameSetting(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool includeChildren);

            ~NodeSoftNameSetting() override = default;

            bool IsVirtualType(const SceneAPI::Containers::Scene& scene, SceneAPI::Containers::SceneGraph::NodeIndex node) const override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            bool MatchesPattern(const SceneAPI::Containers::SceneGraph::Name& name) const;

            bool m_includeChildren = false;
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
