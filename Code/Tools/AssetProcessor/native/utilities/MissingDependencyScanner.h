/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <native/utilities/ApplicationManagerAPI.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }
}

namespace AssetProcessor
{
    class AssetDatabaseConnection;
    class LineByLineDependencyScanner;
    class MissingDependency;
    class PotentialDependencies;
    class SpecializedDependencyScanner;

    typedef AZStd::set<MissingDependency> MissingDependencies;

    enum class ScannerMatchType
    {
        // The scanner to run only matches based on the file extension, such as a "json" scanner will only
        // scan files with the .json extension.
        ExtensionOnlyFirstMatch,
        // The scanners open each file and inspect the contents to see if they look like the format.
        // The first scanner found that matches the file will be used.
        // Example: If a file named "Medium.difficulty" is in XML format, the XML scanner will catch this and scan it.
        FileContentsFirstMatch,
        // All scanners that can scan the given file are used to scan it. Time consuming but thorough.
        Deep
    };

    class MissingDependencyScannerRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::mutex;

        /**
        * Scans the given file for a missing dependency. Note that the database connection is
        * not the AzToolsFramework version, it is the AssetProcessor version. This command needs
        * write access to the database.
        */
        using scanFileCallback = AZStd::function<void(AZStd::string relativeDependencyFilePath)>;
        virtual void ScanFile(
            const AZStd::string& fullPath,
            int maxScanIteration,
            AZ::s64 productPK,
            const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            bool queueDbCommandsOnMainThread,
            scanFileCallback callback) = 0;
    };

    using MissingDependencyScannerRequestBus = AZ::EBus<MissingDependencyScannerRequests>;


    class MissingDependencyScanner
        : public MissingDependencyScannerRequestBus::Handler
        , AssetProcessor::ApplicationManagerNotifications::Bus::Handler
    {
    public:
        MissingDependencyScanner();
        ~MissingDependencyScanner() override;

        // ApplicationManagerNotifications::Bus::Handler
        void ApplicationShutdownRequested() override;

        //! Scans the file at the fullPath for anything that looks like a missing dependency.
        //! Reporting is handled internally, no results are returned.
        //! Anything that matches a result in the given dependency list will not be reported as a missing dependency.
        //! The databaseConnection is used to query the database to transform relative paths into source or
        //! product assets that match those paths, as well as looking up products for UUIDs found in files.
        //! The matchtype is used to determine how to scan the given file, see the ScannerMatchType enum for more information.
        //! A specific scanner can be forced to be used, this will supercede the match type.
        void ScanFile(
            const AZStd::string& fullPath,
            int maxScanIteration,
            AZ::s64 productPK,
            const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            bool queueDbCommandsOnMainThread,
            scanFileCallback callback) override;

        void ScanFile(const AZStd::string& fullPath,
            int maxScanIteration,
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            const AZStd::string& dependencyTokenName,
            bool queueDbCommandsOnMainThread,
            scanFileCallback callback);

        void ScanFile(
            const AZStd::string& fullPath,
            int maxScanIteration,
            AZ::s64 productPK,
            const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            AZStd::string dependencyTokenName,
            ScannerMatchType matchType,
            AZ::Crc32* forceScanner,
            bool queueDbCommandsOnMainThread,
            scanFileCallback callback);

        static const int DefaultMaxScanIteration;

        void RegisterSpecializedScanner(AZStd::shared_ptr<SpecializedDependencyScanner> scanner);

        bool PopulateRulesForScanFolder(const AZStd::string& scanFolderPath, const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AZStd::string& dependencyTokenName);

    protected:
        bool RunScan(
            const AZStd::string& fullPath,
            int maxScanIteration,
            AZ::IO::GenericStream& fileStream,
            PotentialDependencies& potentialDependencies,
            ScannerMatchType matchType,
            AZ::Crc32* forceScanner);

        void PopulateMissingDependencies(
            AZ::s64 productPK,
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
            MissingDependencies& missingDependencies,
            const PotentialDependencies& potentialDependencies);

        void ReportMissingDependencies(
            AZ::s64 productPK,
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            const AZStd::string& dependencyTokenName,
            const MissingDependencies& missingDependencies,
            scanFileCallback callback);

        void SetDependencyScanResultStatus(
            AZStd::string status,
            AZ::s64 productPK,
            const AZStd::string& analysisFingerprint,
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            bool queueDbCommandsOnMainThread,
            scanFileCallback callback);

        typedef AZStd::unordered_map<AZ::Crc32, AZStd::shared_ptr<SpecializedDependencyScanner>> DependencyScannerMap;
        DependencyScannerMap m_specializedScanners;
        AZStd::shared_ptr<LineByLineDependencyScanner> m_defaultScanner;
        AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>> m_dependenciesRulesMap;

        AZStd::atomic_bool m_shutdownRequested = false;
    };
}
