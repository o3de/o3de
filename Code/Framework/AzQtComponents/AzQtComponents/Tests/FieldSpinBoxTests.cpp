/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The edit field of a spin box that is not ours (SpinBox.qss:9-20 names QSpinBox and QDoubleSpinBox): the Gallery's svg label page
    // has two (n2 18_svg-label: the box 363 x 23, its line edit 341 x 19 at (2, 1)). The flattened Style judged only SpinBox and
    // DoubleSpinBox by the sheet's rule, and put the line edit of the others at (3, 3) in 343 x 17, the rect Fusion gives.
    // ----------------------------------------------------------------------------------

    namespace FieldSpinBoxTestsInternal
    {
        using namespace FieldTwins;

        class FieldSpinBoxTests
            : public Fixture
        {
        };

        void buildSpinBoxes(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            auto* layout = new QVBoxLayout(&root);
            auto* integer = new QSpinBox(&root);
            integer->setObjectName(QStringLiteral("integer"));
            integer->setFixedWidth(363);
            layout->addWidget(integer);
            auto* real = new QDoubleSpinBox(&root);
            real->setObjectName(QStringLiteral("real"));
            real->setFixedWidth(363);
            layout->addWidget(real);
            layout->addStretch();
        }

        // ----------------------------------------------------------------------------------
        // T-SP.1: the edit field is the rect of the sheet's rule, whatever Qt class the spin box is
        // ----------------------------------------------------------------------------------

        TEST_F(FieldSpinBoxTests, EditField_OfAnyQtSpinBoxIsTheRectOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildSpinBoxes, QSize(400, 120));

            for (const char* name : { "integer", "real" })
            {
                auto* legacy = twins.legacyChild<QAbstractSpinBox>(name);
                auto* flat = twins.flatChild<QAbstractSpinBox>(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                const QLineEdit* legacyEdit = legacy->findChild<QLineEdit*>();
                const QLineEdit* flatEdit = flat->findChild<QLineEdit*>();
                ASSERT_TRUE(legacyEdit != nullptr && flatEdit != nullptr) << name;

                EXPECT_EQ(flat->height(), legacy->height()) << name;
                EXPECT_TRUE(flatEdit->geometry() == legacyEdit->geometry())
                    << name << ": " << rectText(flatEdit->geometry()) << " against " << rectText(legacyEdit->geometry());

                // The box of the rule: 2px of margin, padding-top -1 and padding-bottom 1, then padding-right 12 and the 6px button
                // (SpinBox.qss:13, :16, :18-19, :81): 341 x 19 at (2, 1) in 363 x 23
                EXPECT_TRUE(flatEdit->geometry() == QRect(2, 1, flat->width() - 22, flat->height() - 4)) << name << ": " << rectText(flatEdit->geometry());
            }
        }
    } // namespace FieldSpinBoxTestsInternal
} // namespace AzQtComponents
