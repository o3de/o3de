/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Common/RPITestFixture.h>
#include <Common/ErrorMessageFinder.h>

#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    //! Test suite for the internal components of MaterialVersionUpdates.
    //! Testing full update functionality in combination with MaterialTypeAssets
    //! and MaterialAssets is performed in their respective test suites.
    class MaterialVersionUpdateTests
        : public RPITestFixture
    {
    protected:
        const MaterialPropertyValue m_invalidValue = MaterialVersionUpdate::Action::s_invalidValue;
        const AZ::Name m_invalidName = MaterialVersionUpdate::MaterialPropertyValueWrapper::s_invalidName;
    };

    TEST_F(MaterialVersionUpdateTests, MaterialPropertyValueWrapper)
    {
        const MaterialPropertyValue intValue(123);
        MaterialVersionUpdate::MaterialPropertyValueWrapper intWrapper(intValue);
        EXPECT_EQ(intWrapper.Get(), intValue);

        const MaterialPropertyValue strValue(AZStd::string("myString"));
        MaterialVersionUpdate::MaterialPropertyValueWrapper strWrapper(strValue);
        EXPECT_EQ(strWrapper.Get(), strValue);
        EXPECT_EQ(strWrapper.GetAsName(), AZ::Name(strValue.GetValue<AZStd::string>()));
    }

    TEST_F(MaterialVersionUpdateTests, MaterialPropertyValueWrapper_Error_GetAsName)
    {
        // Empty string should not trigger error
        const MaterialPropertyValue strValue(AZStd::string(""));
        MaterialVersionUpdate::MaterialPropertyValueWrapper strWrapper(strValue);
        EXPECT_EQ(strWrapper.GetAsName(), AZ::Name(strValue.GetValue<AZStd::string>()));

        // Non-string value should trigger error
        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("GetAsName() expects a valid string value");
        const MaterialPropertyValue intValue(123);
        MaterialVersionUpdate::MaterialPropertyValueWrapper intWrapper(intValue);
        EXPECT_EQ(intWrapper.GetAsName(), m_invalidName);
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialVersionUpdateTests, Action_Rename)
    {
        // Test alternative ways of creating the same action
        const AZStd::string fromStr = "oldName";
        const AZStd::string toStr = "newName";
        MaterialVersionUpdate::Action action(
            AZ::Name{ "rename" },
            {
                { Name{ "from" }, fromStr },
                { Name{ "to"   }, toStr }
            } );

        MaterialVersionUpdate::Action action2(
            {
                { AZStd::string("op"),   AZStd::string("rename") },
                { AZStd::string("from"), fromStr },
                { AZStd::string("to"),   toStr }
            } );

        MaterialVersionUpdate::Action action3(AZ::Name("rename"), {});
        action3.AddArg(Name{ "from" }, fromStr);
        action3.AddArg(Name{ "to"   }, toStr);

        EXPECT_EQ(action, action2);
        EXPECT_EQ(action, action3);
        EXPECT_TRUE(action.Validate());

        // Test properties
        EXPECT_EQ(action.GetArgCount(), 2);
        EXPECT_EQ(action.GetArg(Name{ "from" }), fromStr);
        EXPECT_EQ(action.GetArg(Name{ "to"   }), toStr);
        EXPECT_EQ(action.GetArgAsName(Name{ "from" }), AZ::Name(fromStr));
        EXPECT_EQ(action.GetArgAsName(Name{ "to"   }), AZ::Name(toStr));
    }

    TEST_F(MaterialVersionUpdateTests, Action_RenamePrefix)
    {
        // Test alternative ways of creating the same action
        const AZStd::string fromStr = "oldPrefix_";
        const AZStd::string toStr = "newPrefix.";
        MaterialVersionUpdate::Action action(
            AZ::Name{"renamePrefix"},
            {
                { Name{ "from" }, fromStr },
                { Name{ "to"   }, toStr }
            });

        MaterialVersionUpdate::Action action2(
            {
                { AZStd::string("op"),   AZStd::string("renamePrefix") },
                { AZStd::string("from"), fromStr },
                { AZStd::string("to"),   toStr }
            });

        MaterialVersionUpdate::Action action3(AZ::Name("renamePrefix"), {});
        action3.AddArg(Name{"from"}, fromStr);
        action3.AddArg(Name{"to"}, toStr);

        EXPECT_EQ(action, action2);
        EXPECT_EQ(action, action3);
        EXPECT_TRUE(action.Validate());

        // Test properties
        EXPECT_EQ(action.GetArgCount(), 2);
        EXPECT_EQ(action.GetArg(Name{"from"}), fromStr);
        EXPECT_EQ(action.GetArg(Name{"to"}), toStr);
        EXPECT_EQ(action.GetArgAsName(Name{"from"}), AZ::Name(fromStr));
        EXPECT_EQ(action.GetArgAsName(Name{"to"}), AZ::Name(toStr));
    }

    TEST_F(MaterialVersionUpdateTests, Action_SetValue)
    {
        // Test alternative ways of creating the same action
        const AZStd::string nameStr = "myInt";
        const MaterialPropertyValue theValue(123);
        MaterialVersionUpdate::Action action(
            AZ::Name{ "setValue" },
            {
                { Name{ "name"  }, nameStr },
                { Name{ "value" }, theValue }
            } );

        MaterialVersionUpdate::Action action2(
            {
                { AZStd::string("op"),    AZStd::string("setValue") },
                { AZStd::string("name"),  nameStr },
                { AZStd::string("value"), theValue }
            } );

        MaterialVersionUpdate::Action action3(AZ::Name("setValue"), {});
        action3.AddArg(Name{ "name"  }, nameStr);
        action3.AddArg(Name{ "value" }, theValue);

        EXPECT_EQ(action, action2);
        EXPECT_EQ(action, action3);
        EXPECT_TRUE(action.Validate());

        // Test properties
        EXPECT_EQ(action.GetArgCount(), 2);
        EXPECT_EQ(action.GetArg(Name{ "name"  }), nameStr);
        EXPECT_EQ(action.GetArg(Name{ "value" }), theValue);
        EXPECT_EQ(action.GetArgAsName(Name{ "name" }), AZ::Name(nameStr));
    }

    TEST_F(MaterialVersionUpdateTests, Action_Error_GetArg)
    {
        const AZStd::string nameStr = "myInt";
        const MaterialPropertyValue theValue(123);
        MaterialVersionUpdate::Action action(
            AZ::Name{ "setValue" },
            {
                { Name{ "name"  }, nameStr },
                { Name{ "value" }, theValue }
            } );

        // Non-existent key
        EXPECT_EQ(action.GetArg(AZ::Name("invalidKey")), m_invalidValue);

        // GetArgAsName with non-string value
        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("expects a valid string value");
        EXPECT_EQ(action.GetArgAsName(AZ::Name("value")), m_invalidName);
    }
}

