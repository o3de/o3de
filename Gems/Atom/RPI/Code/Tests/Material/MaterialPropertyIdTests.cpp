/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/ErrorMessageFinder.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialPropertyIdTests
        : public RPITestFixture
    {
    };

    TEST_F(MaterialPropertyIdTests, TestConstructWithPropertyName)
    {
        MaterialPropertyId id{"color"};
        EXPECT_TRUE(id.IsValid());
        EXPECT_STREQ(id.GetCStr(), "color");
        AZ::Name idCastedToName = id;
        EXPECT_EQ(idCastedToName, AZ::Name{"color"});
    }

    TEST_F(MaterialPropertyIdTests, TestConstructWithPropertyName_BadName)
    {
        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("not a valid identifier");

        MaterialPropertyId id{"color?"};
        EXPECT_FALSE(id.IsValid());

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialPropertyIdTests, TestConstructWithTwoNames)
    {
        MaterialPropertyId id{"baseColor", "factor"};
        EXPECT_TRUE(id.IsValid());
        EXPECT_STREQ(id.GetCStr(), "baseColor.factor");
        AZ::Name idCastedToName = id;
        EXPECT_EQ(idCastedToName, AZ::Name{"baseColor.factor"});
    }
    
    TEST_F(MaterialPropertyIdTests, TestConstructWithTwoNames_BadGroupName)
    {
        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("not a valid identifier");
        
        MaterialPropertyId id{"layer1.baseColor", "factor"};
        EXPECT_FALSE(id.IsValid());

        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialPropertyIdTests, TestConstructWithTwoNames_BadPropertyName)
    {
        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("not a valid identifier");
        
        MaterialPropertyId id{"baseColor", ".factor"};
        EXPECT_FALSE(id.IsValid());

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialPropertyIdTests, TestConstructWithMultipleNames)
    {
        AZStd::vector<AZStd::string> names{"layer1", "clearCoat", "normal", "factor"};
        MaterialPropertyId id{names};
        EXPECT_TRUE(id.IsValid());
        EXPECT_STREQ(id.GetCStr(), "layer1.clearCoat.normal.factor");
        AZ::Name idCastedToName = id;
        EXPECT_EQ(idCastedToName, AZ::Name{"layer1.clearCoat.normal.factor"});
    }
    
    TEST_F(MaterialPropertyIdTests, TestConstructWithMultipleNames_BadName)
    {
        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("not a valid identifier");
        
        AZStd::vector<AZStd::string> names{"layer1", "clear-coat", "normal", "factor"};
        MaterialPropertyId id{names};
        EXPECT_FALSE(id.IsValid());

        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialPropertyIdTests, TestParse)
    {
        MaterialPropertyId id = MaterialPropertyId::Parse("layer1.clearCoat.normal.factor");
        EXPECT_TRUE(id.IsValid());
        EXPECT_STREQ(id.GetCStr(), "layer1.clearCoat.normal.factor");
        AZ::Name idCastedToName = id;
        EXPECT_EQ(idCastedToName, AZ::Name{"layer1.clearCoat.normal.factor"});
    }
    
    TEST_F(MaterialPropertyIdTests, TestParse_BadName)
    {
        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("not a valid identifier");

        MaterialPropertyId id = MaterialPropertyId::Parse("layer1.clearCoat.normal,factor");
        EXPECT_FALSE(id.IsValid());
        
        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialPropertyIdTests, TestNameValidity)
    {
        EXPECT_TRUE(MaterialPropertyId::IsValidName("a"));
        EXPECT_TRUE(MaterialPropertyId::IsValidName("z"));
        EXPECT_TRUE(MaterialPropertyId::IsValidName("A"));
        EXPECT_TRUE(MaterialPropertyId::IsValidName("Z"));
        EXPECT_TRUE(MaterialPropertyId::IsValidName("_"));
        EXPECT_TRUE(MaterialPropertyId::IsValidName("m_layer10bazBAZ"));
        EXPECT_FALSE(MaterialPropertyId::IsValidName(""));
        EXPECT_FALSE(MaterialPropertyId::IsValidName("1layer"));
        EXPECT_FALSE(MaterialPropertyId::IsValidName("base-color"));
        EXPECT_FALSE(MaterialPropertyId::IsValidName("base.color"));
        EXPECT_FALSE(MaterialPropertyId::IsValidName("base/color"));
    }
}
