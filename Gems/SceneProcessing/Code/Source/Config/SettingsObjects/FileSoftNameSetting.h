/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <initializer_list>
#include <Config/SettingsObjects/SoftNameSetting.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        class FileSoftNameSetting : public SoftNameSetting
        {
        public:
            class GraphType
            {
            public:
                AZ_CLASS_ALLOCATOR(GraphType, AZ::SystemAllocator, 0);
                AZ_RTTI(GraphType, "{368E85F4-4FF5-4708-82A1-FCDC993D4C34}");

                GraphType();
                explicit GraphType(const AZStd::string& name);
                explicit GraphType(AZStd::string&& name);
                virtual ~GraphType() = default;

                const AZStd::string& GetName() const;
                const Uuid& GetId() const;

                static void Reflect(AZ::ReflectContext* context);

            private:
                AZStd::string m_name;
                mutable Uuid m_cachedId;
            };

            // Wrapper around AZStd::vector<GraphType> for the sole purpose of forcing the reflected
            //  property editor to not use a container view.
            class GraphTypeContainer
            {
            public:
                AZ_CLASS_ALLOCATOR(GraphTypeContainer, AZ::SystemAllocator, 0);
                AZ_RTTI(GraphTypeContainer, "{35E70739-CD31-43C2-A024-769755A26CAE}");

                GraphTypeContainer() = default;
                explicit GraphTypeContainer(std::initializer_list<GraphType> graphTypes);
                virtual ~GraphTypeContainer() = default;

                AZStd::vector<GraphType>& GetGraphTypes();
                const AZStd::vector<GraphType>& GetGraphTypes() const;

                static void Reflect(AZ::ReflectContext* context);

            private:
                AZStd::vector<GraphType> m_types;
            };

            AZ_CLASS_ALLOCATOR(FileSoftNameSetting, AZ::SystemAllocator, 0);
            AZ_RTTI(FileSoftNameSetting, "{CED5FBF7-F74A-49E2-9FE0-DFF7EDA274CE}", SoftNameSetting);

            FileSoftNameSetting() = default;
            FileSoftNameSetting(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool inclusive, std::initializer_list<GraphType> graphTypes);
            ~FileSoftNameSetting() override = default;

            bool IsVirtualType(const SceneAPI::Containers::Scene& scene, SceneAPI::Containers::SceneGraph::NodeIndex node) const override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            GraphTypeContainer m_graphTypes;
            bool m_inclusiveList;

            mutable const SceneAPI::Containers::Scene* m_cachedScene;
            mutable bool m_cachedNameMatch;
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
