/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyleManager.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <UI/EntityPropertyEditorTestFixture.h>

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QHash>
#include <QLabel>
#include <QPalette>
#include <QSet>
#include <QWidget>

#include <utility>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The palettes of the inspector's six header labels. EntityPropertyEditor.ui held them (six <palette>s, 63 colour elements); the
    // constructor sets them in code after setupUi, from named constants. The flattened GUI resets them afterwards (to the text colour
    // the labels inherit), so the palettes are the legacy GUI's to hold.
    // ----------------------------------------------------------------------------------

    namespace EntityPropertyEditorPaletteTestsInternal
    {
        //! One <colorrole> of a <palette> of EntityPropertyEditor.ui as the form held it: the widget it was on, the colour group
        //! (<active>, <inactive> or <disabled>), the role, its SolidPattern brush's colour, and the line of the <colorrole> in the .ui
        //! (the <color> is two lines below it). Each of the 63 colour elements of the form is one row.
        struct HeldRole
        {
            const char* m_widget;
            QPalette::ColorGroup m_group;
            QPalette::ColorRole m_role;
            int m_red;
            int m_green;
            int m_blue;
            int m_alpha;
            int m_uiLine;
        };

        constexpr HeldRole HeldRoles[] = {
            // m_entityNameLabel
            { "m_entityNameLabel", QPalette::Active, QPalette::WindowText, 255, 153, 0, 255, 81 },
            { "m_entityNameLabel", QPalette::Inactive, QPalette::WindowText, 255, 153, 0, 255, 92 },
            { "m_entityNameLabel", QPalette::Disabled, QPalette::WindowText, 120, 120, 120, 255, 103 },

            // m_entityIdLabel
            { "m_entityIdLabel", QPalette::Active, QPalette::WindowText, 255, 153, 0, 255, 259 },
            { "m_entityIdLabel", QPalette::Active, QPalette::Button, 51, 51, 51, 255, 268 },
            { "m_entityIdLabel", QPalette::Active, QPalette::Base, 51, 51, 51, 255, 277 },
            { "m_entityIdLabel", QPalette::Active, QPalette::Window, 51, 51, 51, 255, 286 },
            { "m_entityIdLabel", QPalette::Inactive, QPalette::WindowText, 255, 153, 0, 255, 297 },
            { "m_entityIdLabel", QPalette::Inactive, QPalette::Button, 51, 51, 51, 255, 306 },
            { "m_entityIdLabel", QPalette::Inactive, QPalette::Base, 51, 51, 51, 255, 315 },
            { "m_entityIdLabel", QPalette::Inactive, QPalette::Window, 51, 51, 51, 255, 324 },
            { "m_entityIdLabel", QPalette::Disabled, QPalette::WindowText, 120, 120, 120, 255, 335 },
            { "m_entityIdLabel", QPalette::Disabled, QPalette::Button, 51, 51, 51, 255, 344 },
            { "m_entityIdLabel", QPalette::Disabled, QPalette::Base, 51, 51, 51, 255, 353 },
            { "m_entityIdLabel", QPalette::Disabled, QPalette::Window, 51, 51, 51, 255, 362 },

            // m_entityIdText
            { "m_entityIdText", QPalette::Active, QPalette::WindowText, 255, 153, 0, 255, 405 },
            { "m_entityIdText", QPalette::Active, QPalette::Button, 51, 51, 51, 255, 414 },
            { "m_entityIdText", QPalette::Active, QPalette::Base, 51, 51, 51, 255, 423 },
            { "m_entityIdText", QPalette::Active, QPalette::Window, 51, 51, 51, 255, 432 },
            { "m_entityIdText", QPalette::Inactive, QPalette::WindowText, 255, 153, 0, 255, 443 },
            { "m_entityIdText", QPalette::Inactive, QPalette::Button, 51, 51, 51, 255, 452 },
            { "m_entityIdText", QPalette::Inactive, QPalette::Base, 51, 51, 51, 255, 461 },
            { "m_entityIdText", QPalette::Inactive, QPalette::Window, 51, 51, 51, 255, 470 },
            { "m_entityIdText", QPalette::Disabled, QPalette::WindowText, 120, 120, 120, 255, 481 },
            { "m_entityIdText", QPalette::Disabled, QPalette::Button, 51, 51, 51, 255, 490 },
            { "m_entityIdText", QPalette::Disabled, QPalette::Base, 51, 51, 51, 255, 499 },
            { "m_entityIdText", QPalette::Disabled, QPalette::Window, 51, 51, 51, 255, 508 },

            // m_prefabContainerLabel
            { "m_prefabContainerLabel", QPalette::Active, QPalette::WindowText, 255, 153, 0, 255, 622 },
            { "m_prefabContainerLabel", QPalette::Active, QPalette::Button, 51, 51, 51, 255, 631 },
            { "m_prefabContainerLabel", QPalette::Active, QPalette::Base, 51, 51, 51, 255, 640 },
            { "m_prefabContainerLabel", QPalette::Active, QPalette::Window, 51, 51, 51, 255, 649 },
            { "m_prefabContainerLabel", QPalette::Inactive, QPalette::WindowText, 255, 153, 0, 255, 660 },
            { "m_prefabContainerLabel", QPalette::Inactive, QPalette::Button, 51, 51, 51, 255, 669 },
            { "m_prefabContainerLabel", QPalette::Inactive, QPalette::Base, 51, 51, 51, 255, 678 },
            { "m_prefabContainerLabel", QPalette::Inactive, QPalette::Window, 51, 51, 51, 255, 687 },
            { "m_prefabContainerLabel", QPalette::Disabled, QPalette::WindowText, 120, 120, 120, 255, 698 },
            { "m_prefabContainerLabel", QPalette::Disabled, QPalette::Button, 51, 51, 51, 255, 707 },
            { "m_prefabContainerLabel", QPalette::Disabled, QPalette::Base, 51, 51, 51, 255, 716 },
            { "m_prefabContainerLabel", QPalette::Disabled, QPalette::Window, 51, 51, 51, 255, 725 },

            // m_prefabContainerText
            { "m_prefabContainerText", QPalette::Active, QPalette::WindowText, 255, 153, 0, 255, 768 },
            { "m_prefabContainerText", QPalette::Active, QPalette::Button, 51, 51, 51, 255, 777 },
            { "m_prefabContainerText", QPalette::Active, QPalette::Base, 51, 51, 51, 255, 786 },
            { "m_prefabContainerText", QPalette::Active, QPalette::Window, 51, 51, 51, 255, 795 },
            { "m_prefabContainerText", QPalette::Inactive, QPalette::WindowText, 255, 153, 0, 255, 806 },
            { "m_prefabContainerText", QPalette::Inactive, QPalette::Button, 51, 51, 51, 255, 815 },
            { "m_prefabContainerText", QPalette::Inactive, QPalette::Base, 51, 51, 51, 255, 824 },
            { "m_prefabContainerText", QPalette::Inactive, QPalette::Window, 51, 51, 51, 255, 833 },
            { "m_prefabContainerText", QPalette::Disabled, QPalette::WindowText, 120, 120, 120, 255, 844 },
            { "m_prefabContainerText", QPalette::Disabled, QPalette::Button, 51, 51, 51, 255, 853 },
            { "m_prefabContainerText", QPalette::Disabled, QPalette::Base, 51, 51, 51, 255, 862 },
            { "m_prefabContainerText", QPalette::Disabled, QPalette::Window, 51, 51, 51, 255, 871 },

            // m_statusLabel
            { "m_statusLabel", QPalette::Active, QPalette::WindowText, 255, 153, 0, 255, 998 },
            { "m_statusLabel", QPalette::Active, QPalette::Button, 51, 51, 51, 255, 1007 },
            { "m_statusLabel", QPalette::Active, QPalette::Base, 51, 51, 51, 255, 1016 },
            { "m_statusLabel", QPalette::Active, QPalette::Window, 51, 51, 51, 255, 1025 },
            { "m_statusLabel", QPalette::Inactive, QPalette::WindowText, 255, 153, 0, 255, 1036 },
            { "m_statusLabel", QPalette::Inactive, QPalette::Button, 51, 51, 51, 255, 1045 },
            { "m_statusLabel", QPalette::Inactive, QPalette::Base, 51, 51, 51, 255, 1054 },
            { "m_statusLabel", QPalette::Inactive, QPalette::Window, 51, 51, 51, 255, 1063 },
            { "m_statusLabel", QPalette::Disabled, QPalette::WindowText, 120, 120, 120, 255, 1074 },
            { "m_statusLabel", QPalette::Disabled, QPalette::Button, 51, 51, 51, 255, 1083 },
            { "m_statusLabel", QPalette::Disabled, QPalette::Base, 51, 51, 51, 255, 1092 },
            { "m_statusLabel", QPalette::Disabled, QPalette::Window, 51, 51, 51, 255, 1101 },
        };
        static_assert(sizeof(HeldRoles) / sizeof(HeldRoles[0]) == 63, "the form held 63 colour elements");

        //! Records the palette each named widget holds when its first polish begins, before its style can change the palette
        class FirstPolishPalettes
            : public QObject
        {
        public:
            explicit FirstPolishPalettes(QSet<QString> names)
                : m_names(std::move(names))
            {
            }

            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (event->type() == QEvent::Polish && watched->isWidgetType())
                {
                    const QString name = watched->objectName();
                    if (m_names.contains(name) && !m_palettes.contains(name))
                    {
                        m_palettes.insert(name, static_cast<QWidget*>(watched)->palette());
                    }
                }
                return false;
            }

            QSet<QString> m_names;
            QHash<QString, QPalette> m_palettes;
        };
    } // namespace EntityPropertyEditorPaletteTestsInternal

    class EntityPropertyEditorPaletteTests : public EntityPropertyEditorTestFixture
    {
    };

    //! The twin: beside each header label of the inspector, a control label of the same class under the same parent, given the
    //! palette the form held (HeldRoles, quoted from EntityPropertyEditor.ui) and the same sheet, then both polished. A label is
    //! polished long before it is shown (QLabel::minimumSizeHint calls ensurePolished), and polish may replace roles the form set,
    //! so the palette the form held is not what a label ends with: the control ends with what the form's palette became, and the
    //! inspector's label must end with the same, in every role of every group the form set.
    TEST_F(EntityPropertyEditorPaletteTests, TheHeaderLabelsEndWithThePalettesTheFormGaveThem)
    {
        using namespace EntityPropertyEditorPaletteTestsInternal;

        if (AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            GTEST_SKIP() << "the flattened GUI resets these palettes after setting them";
        }

        const auto inspector = AZStd::make_unique<AzToolsFramework::EntityPropertyEditor>(nullptr, Qt::WindowFlags(), false);

        // ---- One control per label: the form's palette, the label's own sheet, the label's parent
        QHash<QString, QLabel*> controls;
        for (const HeldRole& held : HeldRoles)
        {
            const QString name = QString::fromLatin1(held.m_widget);
            if (controls.contains(name))
            {
                continue;
            }
            QLabel* label = inspector->findChild<QLabel*>(name);
            ASSERT_NE(label, nullptr) << held.m_widget;

            QPalette formPalette;
            for (const HeldRole& role : HeldRoles)
            {
                if (name == QLatin1String(role.m_widget))
                {
                    formPalette.setBrush(role.m_group, role.m_role, QBrush(QColor(role.m_red, role.m_green, role.m_blue, role.m_alpha)));
                }
            }

            auto* control = new QLabel(label->text(), label->parentWidget());
            control->setPalette(formPalette);
            control->setStyleSheet(label->styleSheet());
            controls.insert(name, control);
        }

        // ---- Both polished, then every role the form set compared
        for (auto it = controls.cbegin(); it != controls.cend(); ++it)
        {
            QLabel* label = inspector->findChild<QLabel*>(it.key());
            label->ensurePolished();
            it.value()->ensurePolished();
        }
        for (const HeldRole& held : HeldRoles)
        {
            const QLabel* label = inspector->findChild<QLabel*>(QString::fromLatin1(held.m_widget));
            const QLabel* control = controls.value(QString::fromLatin1(held.m_widget));
            EXPECT_EQ(label->palette().brush(held.m_group, held.m_role), control->palette().brush(held.m_group, held.m_role))
                << held.m_widget << " EntityPropertyEditor.ui:" << held.m_uiLine;
        }
    }

    //! The palette the constructor sets is the form's: each label's palette when its first polish begins (QEvent::Polish reaches an
    //! application filter before QWidget::event hands the widget to its style), or after the constructor for a label it did not
    //! polish, holds exactly the colour of the form in every role and group the form set
    TEST_F(EntityPropertyEditorPaletteTests, EachHeaderLabelHoldsTheFormsPaletteUntilItsFirstPolish)
    {
        using namespace EntityPropertyEditorPaletteTestsInternal;

        if (AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            GTEST_SKIP() << "the flattened GUI resets these palettes after setting them";
        }

        QSet<QString> names;
        for (const HeldRole& held : HeldRoles)
        {
            names.insert(QString::fromLatin1(held.m_widget));
        }

        FirstPolishPalettes recorder(names);
        qApp->installEventFilter(&recorder);
        const auto inspector = AZStd::make_unique<AzToolsFramework::EntityPropertyEditor>(nullptr, Qt::WindowFlags(), false);
        qApp->removeEventFilter(&recorder);

        for (const HeldRole& held : HeldRoles)
        {
            const QString name = QString::fromLatin1(held.m_widget);
            const QWidget* widget = inspector->findChild<QWidget*>(name);
            ASSERT_NE(widget, nullptr) << held.m_widget;

            const QPalette palette = recorder.m_palettes.contains(name) ? recorder.m_palettes.value(name) : widget->palette();
            const QBrush brush = palette.brush(held.m_group, held.m_role);
            EXPECT_EQ(brush.style(), Qt::SolidPattern) << held.m_widget << " EntityPropertyEditor.ui:" << held.m_uiLine;
            EXPECT_EQ(brush.color(), QColor(held.m_red, held.m_green, held.m_blue, held.m_alpha))
                << held.m_widget << " EntityPropertyEditor.ui:" << held.m_uiLine;
        }
    }
} // namespace UnitTest
