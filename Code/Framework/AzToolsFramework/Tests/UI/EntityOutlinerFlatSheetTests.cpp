/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerFlatSheet.h>

#include <QFile>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QStyleFactory>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of EntityOutliner.qss: which widgets of the Entity Outliner they reach, and what they set on them
    // ----------------------------------------------------------------------------------

    namespace EntityOutlinerFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matchers ask for a window by its class name, as
        //! the selectors do, and building a real EntityOutlinerWidget needs the Editor's whole entity system.
        class ClassNamedWidget
            : public QWidget
        {
        public:
            ClassNamedWidget(const char* className, QWidget* parent = nullptr)
                : QWidget(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? this : QWidget::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        constexpr const char* MenuIcon = ":/stylesheet/img/UI20/menu-centered.svg";     // EntityOutliner.qss:11
        constexpr const char* DisplayOptionsElementName = "Editor.Box.OutlinerDisplayOptions";

        class EntityOutlinerFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));

                // Once for the process, as the flattened GUI runs the registrar of a sheet once
                static const bool registered = (AzToolsFramework::EntityOutliner::RegisterFlatSheetElements(), true);
                ASSERT_TRUE(registered);
            }

            void TearDown() override
            {
                m_style.reset();
                LeakDetectionFixture::TearDown();
            }

            //! The flat way: the style on the widget, and the element resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget)
            {
                widget.setStyle(m_style.get());
                AzQtComponents::BoxElement::apply(&widget);
            }

            static const char* elementOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            AZStd::unique_ptr<AzQtComponents::Style> m_style;
        };
    } // namespace EntityOutlinerFlatSheetTestsInternal

    using EntityOutlinerFlatSheetTestsInternal::ClassNamedWidget;
    using EntityOutlinerFlatSheetTestsInternal::DisplayOptionsElementName;
    using EntityOutlinerFlatSheetTestsInternal::EntityOutlinerFlatSheetTests;
    using EntityOutlinerFlatSheetTestsInternal::MenuIcon;

    TEST_F(EntityOutlinerFlatSheetTests, DisplayOptionsButton_BelowTheOutlinerGetsTheIconAndItsSizeAtPolish)
    {
        // The widgets of the legacy capture (UiCaptures n2, Editor 01_main_window): QToolButton#m_display_options <
        // QFrame#textSearchContainer < EntityOutlinerSearchWidget#m_searchWidget < ... < EntityOutlinerWidget. Native draws the
        // menu icon there (three bars, 16px); the flat capture draws nothing.
        ClassNamedWidget outliner("AzToolsFramework::EntityOutlinerWidget");
        QFrame* container = new QFrame(&outliner);
        container->setObjectName("textSearchContainer");
        QToolButton* button = new QToolButton(container);
        button->setObjectName("m_display_options");
        button->setIconSize(QSize(32, 32));                 // not the sheet's size

        flatten(*button);

        EXPECT_STREQ(elementOf(*button), DisplayOptionsElementName);
        EXPECT_EQ(button->iconSize(), QSize(16, 16));       // qproperty-iconSize: 16px 16px

        // qproperty-icon: the same file. It is a resource of AzQtComponents; were it missing, both sides below would be an empty image
        ASSERT_TRUE(QFile::exists(QString::fromUtf8(MenuIcon)));
        const QImage expected = QIcon(MenuIcon).pixmap(16, 16).toImage();
        ASSERT_FALSE(expected.isNull());
        EXPECT_FALSE(button->icon().isNull());
        EXPECT_TRUE(button->icon().pixmap(16, 16).toImage() == expected);
    }

    TEST_F(EntityOutlinerFlatSheetTests, DisplayOptionsButton_IsFoundOnlyBelowTheOutlinerAndOnlyByThatNameAndClass)
    {
        // The same name in a window that is not the outliner, another name in it, and a widget that is not a button
        QWidget elsewhere;
        QToolButton* strayButton = new QToolButton(&elsewhere);
        strayButton->setObjectName("m_display_options");

        ClassNamedWidget outliner("AzToolsFramework::EntityOutlinerWidget");
        QToolButton* otherName = new QToolButton(&outliner);
        otherName->setObjectName("m_filter_options");
        QWidget* notAButton = new QWidget(&outliner);
        notAButton->setObjectName("m_display_options");

        for (QWidget* widget : { static_cast<QWidget*>(strayButton), static_cast<QWidget*>(otherName), notAButton })
        {
            flatten(*widget);
        }

        // A Global element of AzQtComponents may cover a tool button or a widget, never this one
        EXPECT_STRNE(elementOf(*strayButton), DisplayOptionsElementName);
        EXPECT_STRNE(elementOf(*otherName), DisplayOptionsElementName);
        EXPECT_STRNE(elementOf(*notAButton), DisplayOptionsElementName);
        EXPECT_TRUE(strayButton->icon().isNull());
        EXPECT_TRUE(otherName->icon().isNull());
    }

    TEST_F(EntityOutlinerFlatSheetTests, Tree_BelowTheOutlinerHasNoSelectionOrHoverFillOfItsOwnAndOnlyThere)
    {
        using AzQtComponents::ItemViewElement;

        // n2 Editor 01_main_window: EntityOutlinerWidget > ... > EntityOutlinerTreeView#m_objectTree (a QTreeView). The sheet makes the role of
        // the selection and the fill of a selected or hovered item and branch transparent: the delegate paints them itself
        ClassNamedWidget outliner("AzToolsFramework::EntityOutlinerWidget");
        QWidget* middle = new QWidget(&outliner);
        QTreeView* tree = new QTreeView(middle);
        tree->setObjectName("m_objectTree");
        QWidget elsewhere;
        QTreeView* strayTree = new QTreeView(&elsewhere);
        strayTree->setObjectName("m_objectTree");

        ItemViewElement::apply(tree);
        ItemViewElement::apply(strayTree);

        const ItemViewElement* element = ItemViewElement::forWidget(tree);
        ASSERT_TRUE(element != nullptr);
        EXPECT_STREQ(element->m_name, "Editor.ItemView.OutlinerTree");
        EXPECT_EQ(element->m_selection.alpha(), 0);
        EXPECT_EQ(tree->palette().color(QPalette::Highlight).alpha(), 0);
        for (const auto scope : { ItemViewElement::OnItem, ItemViewElement::OnBranch })
        {
            EXPECT_EQ(element->fillOf(ItemViewElement::Selected, scope).alpha(), 0);
            EXPECT_EQ(element->fillOf(ItemViewElement::Selected | ItemViewElement::Active, scope).alpha(), 0);
            EXPECT_EQ(element->fillOf(ItemViewElement::Hover, scope).alpha(), 0);
            // a selected item that is hovered, in an inactive window: nothing of the Global greys shows either
            EXPECT_EQ(element->fillOf(ItemViewElement::Selected | ItemViewElement::Hover | ItemViewElement::Inactive, scope).alpha(), 0);
        }

        // What the rules do not name is the Global tree's: the indentation, the caret, the item box and the disabled text
        EXPECT_EQ(element->m_indentation, 20);
        EXPECT_STREQ(element->m_closed.m_image, ":/TreeView/closed.svg");
        EXPECT_EQ(element->m_itemHeight, 24);
        EXPECT_TRUE(element->textOf(ItemViewElement::Disabled) == QColor(0x77, 0x77, 0x77));

        // A tree of another window has the Global element's greys
        const ItemViewElement* globalElement = ItemViewElement::forWidget(strayTree);
        ASSERT_TRUE(globalElement != nullptr);
        EXPECT_STREQ(globalElement->m_name, "Global.ItemView.Tree");
        EXPECT_TRUE(globalElement->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == QColor(101, 101, 101));
        EXPECT_TRUE(globalElement->fillOf(ItemViewElement::Hover, ItemViewElement::OnItem) == QColor(90, 90, 90));
    }
} // namespace UnitTest
