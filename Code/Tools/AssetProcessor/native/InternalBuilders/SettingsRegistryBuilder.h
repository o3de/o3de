/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AssetProcessor
{
    class SettingsRegistryBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        class SettingsExporter : public AZ::SettingsRegistryInterface::Visitor
        {
        public:
            SettingsExporter(rapidjson::StringBuffer& buffer, const AZStd::vector<AZStd::string>& excludes);
            ~SettingsExporter() override = default;

            AZ::SettingsRegistryInterface::VisitResponse Traverse(AZStd::string_view path, AZStd::string_view valueName,
                AZ::SettingsRegistryInterface::VisitAction action, AZ::SettingsRegistryInterface::Type type) override;
            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value) override;
            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value) override;
            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::u64 value) override;
            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, double value) override;
            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value) override;

            bool Finalize();
            void Reset(rapidjson::StringBuffer& buffer);
            
        private:
            rapidjson::Writer<rapidjson::StringBuffer> m_writer;

            const AZStd::vector<AZStd::string>& m_excludes;
            AZStd::stack<bool> m_includeNameStack;
            bool m_includeName{ false };
            bool m_result{ true };

            void WriteName(AZStd::string_view name);
        };

        SettingsRegistryBuilder();
        ~SettingsRegistryBuilder() override = default;

        bool Initialize();
        void Uninitialize();

        void ShutDown() override;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

    protected:
        AZStd::vector<AZStd::string> ReadExcludesFromRegistry() const;

    private:
        AZ::Uuid m_builderId;
        AZ::Data::AssetType m_assetType;
        bool m_isShuttingDown{ false };
    };
} // namespace AssetProcessor
