/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>

namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
    }
    namespace SceneProcessingConfig
    {
        class SoftNameSetting
        {
        public:
            AZ_CLASS_ALLOCATOR(SoftNameSetting, SystemAllocator);
            AZ_RTTI(SoftNameSetting, "{FE7AAAF6-8BA5-4599-B9A6-CC28026A6FFE}");

            SoftNameSetting() = default;
            SoftNameSetting(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType);
            virtual ~SoftNameSetting() = 0;

            virtual const AZStd::string& GetVirtualType() const;
            virtual Crc32 GetVirtualTypeHash() const;

            virtual bool IsVirtualType(const SceneAPI::Containers::Scene& scene, SceneAPI::Containers::SceneGraph::NodeIndex node) const = 0;

            virtual const AZ::Uuid GetTypeId() const = 0;

            static void Reflect(ReflectContext* context);

        protected:
            AZStd::vector<AZStd::string> GetAllVirtualTypes() const;

            SceneAPI::SceneCore::PatternMatcher m_pattern;
            AZStd::string m_virtualType;
            mutable Crc32 m_virtualTypeHash;
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
