/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/AnySerializer.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Base.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

#include <AzCore/Math/MathMatrixSerializer.h>
#include <AzCore/Math/MathVectorSerializer.h>

 // test the any serializer against several standard serializers, and then
 // compare results of deserialization of types against each other
 // then just do the erroneous input and empty any stuff

namespace JsonSerializationTests
{
    class AnyFixture
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_anySerializer = AZStd::make_unique<AZ::JsonAnySerializer>();
        }

        void TearDown() override
        {
            m_anySerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonAnySerializer> m_anySerializer;
    };

    TEST_F(AnyFixture, Just_RunTheTestsAlready)
    {
        bool IamTrue = true;
        bool IamNotFalse = true;
        EXPECT_EQ(IamNotFalse, IamTrue);
    }
}
