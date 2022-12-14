/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

namespace UnitTest
{
    //! Returns a test folder path 
    AZ::IO::Path GetTestFolderPath();

    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName);

    //! Handler for the trace message bus to suppress errors / warnings that are expected due to testing particular
    //! code branches, so as to avoid filling the test output with traces.
    class ErrorHandler
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        explicit ErrorHandler(const char* errorPattern);
        ~ErrorHandler();
        //! Returns the total number of errors encountered (including those which match the expected pattern).
        int GetErrorCount() const;
        //! Returns the total number of warnings encountered (including those which match the expected pattern).
        int GetWarningCount() const;
        //! Returns the number of errors encountered which matched the expected pattern.
        int GetExpectedErrorCount() const;
        //! Returns the number of warnings encountered which matched the expected pattern.
        int GetExpectedWarningCount() const;
        bool SuppressExpectedErrors(const char* window, const char* message);

        // AZ::Debug::TraceMessageBus
        bool OnPreError(
            const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(
            const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;
    private:
        AZStd::string m_errorPattern;
        int m_errorCount;
        int m_warningCount;
        int m_expectedErrorCount;
        int m_expectedWarningCount;
    };

    //! Helper utility for setting a bool registry value in a test.
    class RegistryTestHelper
    {
    public:
        void SetUp(AZStd::string_view path, bool value);
        void TearDown();

    private:
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry = nullptr;
    };
}

namespace AZ
{
    class Component;

    namespace Test
    {
        //! Creates an entity, assigns the given component to it and then creates a slice asset containing the entity
        AZ::Data::Asset<AZ::SliceAsset> CreateSliceFromComponent(AZ::Component* assetComponent, AZ::Data::AssetId sliceAssetId);
 
        // the Assert Absorber can be used to absorb asserts, errors and warnings during unit tests.
        class AssertAbsorber
            : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            AssertAbsorber();
            ~AssertAbsorber();

            bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
            bool OnAssert(const char* message) override;
            bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
            bool OnError(const char* window, const char* message) override;
            bool OnWarning(const char* window, const char* message) override;
            bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;

            AZ::u32 m_errorCount{ 0 };
            AZ::u32 m_warningCount{ 0 };
            AZ::u32 m_assertCount{ 0 };
        };

    } //namespace Test

}// namespace AZ
