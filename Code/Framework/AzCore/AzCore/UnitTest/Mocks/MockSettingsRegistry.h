/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <gmock/gmock.h>

namespace AZ
{
    class MockSettingsRegistry;
    using NiceSettingsRegistrySimpleMock = ::testing::NiceMock<MockSettingsRegistry>;

    class MockSettingsRegistry
        : public AZ::SettingsRegistryInterface
    {
    public:
        MOCK_CONST_METHOD1(GetType, Type(AZStd::string_view));
        MOCK_CONST_METHOD2(Visit, bool(Visitor&, AZStd::string_view));
        MOCK_CONST_METHOD2(Visit, bool(const VisitorCallback&, AZStd::string_view));
        MOCK_METHOD1(RegisterNotifier, NotifyEventHandler(NotifyCallback));
        MOCK_METHOD1(RegisterNotifier, void(NotifyEventHandler&));
        MOCK_METHOD1(RegisterPreMergeEvent, PreMergeEventHandler(PreMergeEventCallback));
        MOCK_METHOD1(RegisterPreMergeEvent, void(PreMergeEventHandler&));
        MOCK_METHOD1(RegisterPostMergeEvent, PostMergeEventHandler(PostMergeEventCallback));
        MOCK_METHOD1(RegisterPostMergeEvent, void(PostMergeEventHandler&));

        MOCK_CONST_METHOD2(Get, bool(bool&, AZStd::string_view));
        MOCK_CONST_METHOD2(Get, bool(s64&, AZStd::string_view));
        MOCK_CONST_METHOD2(Get, bool(u64&, AZStd::string_view));
        MOCK_CONST_METHOD2(Get, bool(double&, AZStd::string_view));
        MOCK_CONST_METHOD2(Get, bool(AZStd::string&, AZStd::string_view));
        MOCK_CONST_METHOD2(Get, bool(FixedValueString&, AZStd::string_view));
        MOCK_CONST_METHOD3(GetObject, bool(void*, Uuid, AZStd::string_view));

        MOCK_METHOD2(Set, bool(AZStd::string_view, bool));
        MOCK_METHOD2(Set, bool(AZStd::string_view, s64));
        MOCK_METHOD2(Set, bool(AZStd::string_view, u64));
        MOCK_METHOD2(Set, bool(AZStd::string_view, double));
        MOCK_METHOD2(Set, bool(AZStd::string_view, AZStd::string_view));
        MOCK_METHOD2(Set, bool(AZStd::string_view, const char*));
        MOCK_METHOD3(SetObject, bool(AZStd::string_view, const void*, Uuid));

        MOCK_METHOD1(Remove, bool(AZStd::string_view));

        MOCK_METHOD3(MergeCommandLineArgument, bool(AZStd::string_view, AZStd::string_view, const CommandLineArgumentSettings&));
        MOCK_METHOD3(MergeSettings, bool(AZStd::string_view, Format, AZStd::string_view));
        MOCK_METHOD4(MergeSettingsFile, bool(AZStd::string_view, Format, AZStd::string_view, AZStd::vector<char>*));
        MOCK_METHOD5(
            MergeSettingsFolder,
            bool(AZStd::string_view, const Specializations&, AZStd::string_view, AZStd::string_view, AZStd::vector<char>*));

        MOCK_METHOD1(SetApplyPatchSettings, void(const JsonApplyPatchSettings&));
        MOCK_METHOD1(GetApplyPatchSettings, void(JsonApplyPatchSettings&));
        MOCK_METHOD1(SetUseFileIO, void(bool));
    };
} // namespace AZ

