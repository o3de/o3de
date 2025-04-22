/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Platform/PlatformDefaults.h>

namespace AZ
{
    class ReflectionContext;
}

namespace AzToolsFramework
{

    struct DependencyNode;

    class AssetFileDebugInfo
    {
    public:
        AZ_TYPE_INFO(AssetFileDebugInfo, "{1F7C8B0E-4403-49CA-A11F-ACBA05BEBF6A}");
        AZ_CLASS_ALLOCATOR(AssetFileDebugInfo, AZ::SystemAllocator);

        AssetFileDebugInfo() = default;
        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::AssetId m_assetId;
        AZStd::string m_assetRelativePath;
        AZ::IO::SizeType m_fileSize = 0;

        // Direct references to this file.
        // A dependency graph can be built by crawling this.
        AZStd::set<AZ::Data::AssetId> m_filesThatReferenceMe;
    };

    class AssetFileDebugInfoList
    {
    public:
        AZ_TYPE_INFO(AssetFileDebugInfoList, "{FD66D05D-B4F4-4F48-A4E8-FFE231BCC128}");
        AZ_CLASS_ALLOCATOR(AssetFileDebugInfoList, AZ::SystemAllocator);

        AssetFileDebugInfoList() = default;
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetAllProductDependenciesDebug(
            const AZ::Data::AssetId& assetId,
            const AzFramework::PlatformId& platformIndex,
            AssetFileDebugInfoList* debugList,
            AZStd::unordered_set<AZ::Data::AssetId>* cyclicalDependencySet,
            const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
            const AZStd::vector<AZStd::string>& wildcardPatternExclusionList = AZStd::vector<AZStd::string>());

        static const char* GetAssetListDebugFileExtension();

        void BuildHumanReadableString();
        void BuildNodeTree(AZ::Data::AssetId assetId, DependencyNode* parent);

        AZStd::map<AZ::Data::AssetId, AssetFileDebugInfo> m_fileDebugInfoList;
        AZStd::string m_humanReadableString;
    };
} // namespace AzToolsFramework
