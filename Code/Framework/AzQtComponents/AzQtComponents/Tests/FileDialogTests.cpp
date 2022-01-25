/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <QString>

TEST(AzQtComponents, ApplyMissingExtension_UpdateMissingExtension_Success)
{
    const QString textFiler{"Text Files (*.txt)"};
    QString testPath{"testFile"};
    bool result = AzQtComponents::FileDialog::ApplyMissingExtension(textFiler, testPath);
    EXPECT_TRUE(result);
    EXPECT_STRCASEEQ("testFile.txt", testPath.toUtf8().constData());
}

TEST(AzQtComponents, ApplyMissingExtension_NoUpdateExistingExtension_Success)
{
    const QString textFiler{"Text Files (*.txt)"};
    QString testPath{"testFile.txt"};
    bool result = AzQtComponents::FileDialog::ApplyMissingExtension(textFiler, testPath);
    EXPECT_FALSE(result);
    EXPECT_STRCASEEQ("testFile.txt", testPath.toUtf8().constData());
}

TEST(AzQtComponents, ApplyMissingExtension_UpdateMissingExtensionMultipleExtensionFilter_Success)
{
    const QString textFiler{"Image Files (*.jpg *.bmp *.png)"};
    QString testPath{"testFile"};
    bool result = AzQtComponents::FileDialog::ApplyMissingExtension(textFiler, testPath);
    EXPECT_TRUE(result);
    EXPECT_STRCASEEQ("testFile.jpg", testPath.toUtf8().constData());
}

TEST(AzQtComponents, ApplyMissingExtension_NoUpdateMissingExtensionMultipleExtensionFilter_Success)
{
    const QString textFiler{"Image Files (*.jpg *.bmp *.png)"};
    QString testPath{"testFile.png"};
    bool result = AzQtComponents::FileDialog::ApplyMissingExtension(textFiler, testPath);
    EXPECT_FALSE(result);
    EXPECT_STRCASEEQ("testFile.png", testPath.toUtf8().constData());
}

TEST(AzQtComponents, ApplyMissingExtension_NoUpdateMissingExtensionEmptyFilter_Success)
{
    const QString textFiler{""};
    QString testPath{"testFile"};
    bool result = AzQtComponents::FileDialog::ApplyMissingExtension(textFiler, testPath);
    EXPECT_FALSE(result);
    EXPECT_STRCASEEQ("testFile", testPath.toUtf8().constData());
}

TEST(AzQtComponents, ApplyMissingExtension_NoUpdateMissingExtensionInvalidFilter_Success)
{
    const QString textFiler{"Bad Filter!!"};
    QString testPath{"testFile"};
    bool result = AzQtComponents::FileDialog::ApplyMissingExtension(textFiler, testPath);
    EXPECT_FALSE(result);
    EXPECT_STRCASEEQ("testFile", testPath.toUtf8().constData());
}
