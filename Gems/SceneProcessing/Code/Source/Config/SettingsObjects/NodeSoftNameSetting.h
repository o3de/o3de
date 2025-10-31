/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(NodeSoftNameSetting, SystemAllocator);
            AZ_RTTI(NodeSoftNameSetting, "{74629DAE-641A-4BCE-B6D5-3F7DD9F647FA}", SoftNameSetting);

            NodeSoftNameSetting() = default;
            NodeSoftNameSetting(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool includeChildren);

            ~NodeSoftNameSetting() override = default;

            bool IsVirtualType(const SceneAPI::Containers::Scene& scene, SceneAPI::Containers::SceneGraph::NodeIndex node) const override;

            const AZ::Uuid GetTypeId() const override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            bool MatchesPattern(const SceneAPI::Containers::SceneGraph::Name& name) const;

            bool m_includeChildren = false;
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
