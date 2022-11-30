/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "ViewportCameraSelectorWindow_Internals.h"

class CameraEditorUITests
    : public UnitTest::LeakDetectionFixture
{
};

TEST_F(CameraEditorUITests, TestCameraListModelAddAndRemove)
{
    QScopedPointer<Camera::Internal::CameraListModel> model(
        new Camera::Internal::CameraListModel(nullptr));

    int expectedAdds = 0;
    int expectedRemoves = 0;

    QObject::connect(model.get(), &QAbstractItemModel::rowsAboutToBeInserted, [&]() {
        EXPECT_TRUE(expectedAdds > 0);
        expectedAdds -= 1;
    });
    QObject::connect(model.get(), &QAbstractItemModel::rowsAboutToBeRemoved, [&]() {
        EXPECT_TRUE(expectedRemoves > 0);
        expectedRemoves -= 1;
    });

    AZ::EntityId e1{1}, e2{2}, e3{3};

    using CameraNotificationBus = AZ::EBus<Camera::CameraNotifications>;

    expectedAdds = 2;
    CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraAdded, e1);
    CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraAdded, e2);
    EXPECT_TRUE(expectedAdds == 0);
    // There should be three rows, two for our additions and one default editor camera entry
    EXPECT_TRUE(model->rowCount() == 3);

    expectedRemoves = 2;
    CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraRemoved, e1);
    CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraRemoved, e2);
    // We never added e3, so the model should just ignore this
    CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraRemoved, e3);
    EXPECT_TRUE(expectedRemoves == 0);
    EXPECT_TRUE(model->rowCount() == 1);
}
