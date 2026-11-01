/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <QLabel>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The colour tags of a label (Text.qss:11-29): a .ui file sets the class of a label in retranslateUi, after the layout of the form has
    // polished it. The sheet style read the class when the form was reparented, which polished the form's widgets again (QWidgetPrivate::
    // inheritStyle, setStyle_helper, qwidget.cpp:2706-2712, under an application style sheet: the "Default" and "Filled-in" labels of the
    // Gallery's Spin Box page are #888888). The flattened Style polishes a label again when its class changes after its polish.
    // ----------------------------------------------------------------------------------

    namespace FieldLabelClassTestsInternal
    {
        using namespace FieldTwins;

        class FieldLabelClassTests
            : public Fixture
        {
        };

        struct Tag
        {
            const char* m_name;
            const char* m_class;
            const char* m_color;                //!< Text.qss
        };

        const Tag g_tags[] = {
            { "secondary", "secondaryText", "#888888" },
            { "highlighted", "highlightedText", "#44b2f8" },
            { "black", "blackText", "#000000" },
            { "secondaryTwo", "secondaryText", "#888888" },
        };

        void buildLabels(QWidget& root)
        {
            auto* layout = new QVBoxLayout(&root);
            for (const Tag& tag : g_tags)
            {
                auto* label = new QLabel(QStringLiteral("A label"), &root);
                label->setObjectName(QString::fromLatin1(tag.m_name));
                layout->addWidget(label);
            }
            auto* plain = new QLabel(QStringLiteral("A label"), &root);
            plain->setObjectName(QStringLiteral("plain"));
            layout->addWidget(plain);
            layout->addStretch();
        }

        void expectSamePalette(const QLabel* flat, const QLabel* legacy, const char* name)
        {
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text })
                {
                    EXPECT_EQ(flat->palette().color(group, role).name(), legacy->palette().color(group, role).name())
                        << name << ", group " << int(group) << ", role " << int(role);
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T-LC.1: a class set after the polish colours the label as the sheet coloured it
        // ----------------------------------------------------------------------------------

        TEST_F(FieldLabelClassTests, ClassSetAfterThePolish_IsTheColourOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            QWidget legacyRoot;
            buildLabels(legacyRoot);
            for (const Tag& tag : g_tags)
            {
                legacyRoot.findChild<QLabel*>(QString::fromLatin1(tag.m_name))->setProperty("class", QString::fromLatin1(tag.m_class));
            }
            legacy(legacyRoot);
            show(legacyRoot, QSize(200, 200));

            // the flattened form is polished first (it is shown), then its labels get their classes, as retranslateUi gives them
            QWidget flatRoot;
            buildLabels(flatRoot);
            flat(flatRoot);
            show(flatRoot, QSize(200, 200));
            for (const Tag& tag : g_tags)
            {
                auto* label = flatRoot.findChild<QLabel*>(QString::fromLatin1(tag.m_name));
                ASSERT_TRUE(label->testAttribute(Qt::WA_WState_Polished)) << tag.m_name;
                label->setProperty("class", QString::fromLatin1(tag.m_class));
            }

            for (const Tag& tag : g_tags)
            {
                const auto* legacyLabel = legacyRoot.findChild<QLabel*>(QString::fromLatin1(tag.m_name));
                const auto* flatLabel = flatRoot.findChild<QLabel*>(QString::fromLatin1(tag.m_name));
                EXPECT_EQ(legacyLabel->palette().color(QPalette::Active, QPalette::WindowText).name(), QString::fromLatin1(tag.m_color)) << tag.m_name;
                EXPECT_EQ(flatLabel->palette().color(QPalette::Active, QPalette::WindowText).name(), QString::fromLatin1(tag.m_color)) << tag.m_name;
                expectSamePalette(flatLabel, legacyLabel, tag.m_name);
            }

            // a label with no class is the label it was (the application's white)
            expectSamePalette(flatRoot.findChild<QLabel*>(QStringLiteral("plain")), legacyRoot.findChild<QLabel*>(QStringLiteral("plain")), "plain");
        }

        TEST_F(FieldLabelClassTests, ClassSetBeforeThePolish_IsTheSame)
        {
            QWidget flatRoot;
            buildLabels(flatRoot);
            for (const Tag& tag : g_tags)
            {
                flatRoot.findChild<QLabel*>(QString::fromLatin1(tag.m_name))->setProperty("class", QString::fromLatin1(tag.m_class));
            }
            flat(flatRoot);
            show(flatRoot, QSize(200, 200));

            for (const Tag& tag : g_tags)
            {
                const auto* label = flatRoot.findChild<QLabel*>(QString::fromLatin1(tag.m_name));
                EXPECT_EQ(label->palette().color(QPalette::Active, QPalette::WindowText).name(), QString::fromLatin1(tag.m_color)) << tag.m_name;
            }
        }
    } // namespace FieldLabelClassTestsInternal
} // namespace AzQtComponents
