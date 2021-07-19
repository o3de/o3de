/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <MCore/Source/DualQuaternion.h>

namespace MCore
{
    TEST(DualQuaternionTests, ToTransform)
    {
        const AZ::Vector3 translation(0.15f, -1.8f, 0.23f);
        const AZ::Quaternion rotation(0.72f, -0.48f, -0.24f, 0.44f);

        const DualQuaternion dualQuaternion = DualQuaternion::ConvertFromRotationTranslation(rotation, translation);

        const AZ::Transform transform = dualQuaternion.ToTransform();

        EXPECT_TRUE(transform.GetBasisX().IsClose(AZ::Vector3(0.424f, -0.9024f, 0.0768f)));
        EXPECT_TRUE(transform.GetBasisY().IsClose(AZ::Vector3(-0.48f, -0.152f, 0.864f)));
        EXPECT_TRUE(transform.GetBasisZ().IsClose(AZ::Vector3(-0.768f, -0.4032f, -0.4976f)));
        EXPECT_TRUE(transform.GetTranslation().IsClose(translation));
    }
} // namespace MCore
