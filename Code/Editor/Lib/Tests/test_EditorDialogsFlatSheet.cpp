/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <EditorFlatSheet.h>

#include <QApplication>
#include <QDialog>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMargins>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of the sheet the Editor Preferences dialog sets on itself, EditorPreferencesDialog.qss:
    // which widgets it reaches, and that they lay out and paint what Qt's own style sheet style laid out and painted, with the
    // sheet as the dialog set it and the rules of the application's sheet that reach the widget, at the device ratios the GUI runs
    // at. The twins are built from the real classes the selectors name (the property editor, its rows and their name labels), not
    // from stand-ins, as a class selector only matches a class the meta object knows.
    //
    // NOT covered: the look of the controls inside the rows of a page (Global elements), and the delegate's own colours for a selected
    // and a hovered page (EditorPreferencesTreeWidgetItemDelegate.cpp:23-24, upstream paint). The application rules quoted below are
    // those of the base application sheet; Editor.qss, which is set on the main window the Preferences dialog is a child of, also
    // reaches it, with two rules for the name of a row (Editor.qss:43-53: a row that can be reordered, a row overridden by a prefab)
    // that no row of a preference page is, so they play no part in these twins.
    // ----------------------------------------------------------------------------------

    namespace EditorDialogsFlatSheetTestsInternal
    {
        constexpr const char* PreferencesDialogName = "EditorPreferencesDialog";
        constexpr const char* DialogElementName = "Editor.Box.PreferencesDialog";
        constexpr const char* PageTreeElementName = "Editor.Box.PreferencesPageTree";
        constexpr const char* PageTreeItemsElementName = "Editor.ItemView.PreferencesPageTree";
        constexpr const char* PropertyEditorElementName = "Editor.Box.PreferencesPropertyEditor";
        constexpr const char* GroupRowElementName = "Editor.Box.PreferencesGroupRow";
        constexpr const char* RowNameElementName = "Editor.Box.PreferencesRowName";
        constexpr const char* GlobalRowNameElementName = "Global.Box.PropertyRowName";

        // EditorPreferencesDialog.qss, whole: the sheet the dialog sets on itself (EditorPreferencesDialog.cpp:99)
        constexpr const char* PreferencesSheet = R"(
QWidget#EditorPreferencesDialog
{
    margin: 0px;
    padding: -4px -5px -4px -5px;
    background-color: #444444;
    border-color: transparent;
}

QWidget#EditorPreferencesDialog QTreeWidget#pageTree
{
    font-size: 11px;
    border: 1px solid #111111;
    background-color: #333333;
    margin: 0px;
}

QWidget#EditorPreferencesDialog QTreeWidget#pageTree::item
{
    margin-top: 3px;
    margin-bottom: 2px;
}

QWidget#EditorPreferencesDialog AzToolsFramework--ReflectedPropertyEditor#propertyEditor
{
    background-color: #222222;
    border: 1px solid #111111;
    margin: 0px 2px 0px 5px
}

AzToolsFramework--PropertyRowWidget QLabel#Name
{
    color: #eeeeee;
    padding: 0 0 0 5px;
}

AzToolsFramework--PropertyRowWidget[HasParent="false"] QLabel#Name
{
    font-weight: bold;
    padding: 0 0 0 5;
    color: white;
}

#EditorPreferencesDialog AzToolsFramework--PropertyRowWidget[isTopLevel="true"][hasChildRows="true"]
{
    border-bottom: 1px solid rgba(255, 255, 255, 0.2);
    padding: 4px 0 6px;
    margin-bottom: 4px;
}

#EditorPreferencesDialog AzToolsFramework--PropertyRowWidget[isTopLevel="true"][hasChildRows="true"] QLabel#Name
{
    font-weight: bold;
}
)";

        // The rules of the application's sheet that reach the widgets, with the lines they are on
        // BaseStyleSheet.qss:15-21, the universal rule
        constexpr const char* UniversalRule =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }\n";
        // BaseStyleSheet.qss:60-66, the colours of a dialog
        constexpr const char* DialogRule = "QMainWindow, QDialog, QDockWidget { color: #cccccc; background-color: #444444; }\n";
        // BaseStyleSheet.qss:23-38, the fill of a view, and its reset for a view in a widget
        constexpr const char* ViewFillRules =
            "QTableView, QListView, QTreeView { background-color: #444444; }\n"
            "QWidget > QTableView, QWidget > QListView, QWidget > QTreeView { background-color: transparent; }\n";
        // TableView.qss:14-29, the roles and the border of a view
        constexpr const char* ViewRule =
            "AzQtComponents--TableView, QTableView, QListView, QTreeView { alternate-background-color: rgb(77, 77, 77); "
            "selection-background-color: rgb(101, 101, 101); selection-color: #ffffff; show-decoration-selected: 1; border: none; }\n";
        // TableView.qss:31-60, the item rules of the Global view
        constexpr const char* ItemRules =
            "AzQtComponents--TableView::item, QTableView::item, QListView::item, QTreeView::item "
            "{ height: 24px; padding-left: 4px; border: none; }\n"
            "AzQtComponents--TableView::branch:selected, AzQtComponents--TableView::item:selected, QTableView::item:selected, "
            "QListView::item:selected, QTreeView::branch:selected, QTreeView::item:selected { background: rgb(101, 101, 101); }\n"
            "AzQtComponents--TableView::branch:hover, AzQtComponents--TableView::item:hover, QTableView::item:hover, "
            "QListView::item:hover, QTreeView::branch:hover, QTreeView::item:hover { background: rgb(90, 90, 90); }\n";
        // QDockWidget.qss:33-36, the margin every scroll area has
        constexpr const char* ScrollAreaRule = "QAbstractScrollArea { margin: 0; }\n";
        // Text.qss:89-95, every label
        constexpr const char* LabelRule =
            ".Label, QLabel { font-family: \"Open Sans\"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }\n";
        // ReflectedPropertyEditor.qss:9-15, the name label of every row
        constexpr const char* RowNameRule =
            "AzToolsFramework--PropertyRowWidget QLabel#Name { min-height: 16px; max-height: 16px; margin: 0px; margin-left: 4px; }\n";

        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey

        constexpr QSize DialogSize(300, 200);
        constexpr QSize TreeSize(100, 80);
        constexpr QSize EditorSize(190, 120);
        constexpr QSize RowSize(150, 40);
        constexpr QSize NameLabelSize(80, 16);          // the name label of a row is 16px high whatever the text needs
        constexpr QSize ProbeSize(50, 20);              // the fixed-size widget a layout of the dialog holds

        //! The option QFrame::paintEvent builds for a frame (QFrame::initStyleOption, protected)
        QStyleOptionFrame frameOption(const QFrame* frame)
        {
            QStyleOptionFrame option;
            option.initFrom(frame);
            option.frameShape = frame->frameShape();
            option.lineWidth = frame->lineWidth();
            option.midLineWidth = frame->midLineWidth();
            if (frame->frameShadow() == QFrame::Sunken)
            {
                option.state |= QStyle::State_Sunken;
            }
            else if (frame->frameShadow() == QFrame::Raised)
            {
                option.state |= QStyle::State_Raised;
            }
            return option;
        }

        //! What a widget paints for its box through its own style, in the order Qt paints it: the styled background (PE_Widget,
        //! QWidgetPrivate::paintBackground), then a frame's frame (CE_ShapedFrame, QFrame::drawFrame). The image is the painter's
        //! device, so the style scales the box to the ratio as a screen of that ratio does.
        QImage paintBox(const QWidget& widget, qreal ratio)
        {
            QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            QPainter painter(&image);
            if (widget.testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(&widget);
                widget.style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, &widget);
            }
            if (const auto* frame = qobject_cast<const QFrame*>(&widget))
            {
                const QStyleOptionFrame option = frameOption(frame);
                widget.style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &widget);
            }
            painter.end();
            return image;
        }

        //! The first pixel two images of one size differ at, as text for a failure message ("" when they are equal)
        std::string firstDifference(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return "the sizes differ";
            }
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    if (a.pixel(x, y) != b.pixel(x, y))
                    {
                        const auto hex = [](QRgb rgb)
                        {
                            return QString::number(rgb, 16).toStdString();
                        };
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #" + hex(a.pixel(x, y)) + ", flat #"
                            + hex(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! The contents box of a frame (SE_ShapedFrameContents): where its contents sit inside its margin, border and padding
        QRect frameContents(const QFrame& frame)
        {
            const QStyleOptionFrame option = frameOption(&frame);
            return frame.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &frame);
        }

        //! The widgets of the Editor Preferences dialog that its sheet reaches, in the dialog the form builds them in:
        //! the list of pages, the property editor, and in it the rows of a page. The group row has a child, so it is a top-level
        //! row with child rows; the child is not a top-level row (its parent has a name), and the leaf row has no child rows.
        struct DialogParts
        {
            QDialog* m_dialog = nullptr;
            QTreeWidget* m_pageTree = nullptr;
            AzToolsFramework::ReflectedPropertyEditor* m_editor = nullptr;
            AzToolsFramework::PropertyRowWidget* m_groupRow = nullptr;
            AzToolsFramework::PropertyRowWidget* m_childRow = nullptr;
            AzToolsFramework::PropertyRowWidget* m_leafRow = nullptr;
            QWidget* m_probe = nullptr;
        };

        AzToolsFramework::PropertyRowWidget* makeRow(QWidget& parent, const char* name)
        {
            auto* row = new AzToolsFramework::PropertyRowWidget(&parent);
            row->SetNameLabel(name);
            row->setGeometry(0, 0, RowSize.width(), RowSize.height());
            row->GetNameLabel()->setGeometry(0, 0, NameLabelSize.width(), NameLabelSize.height());
            return row;
        }

        DialogParts buildDialog(QDialog& dialog)
        {
            DialogParts parts;
            parts.m_dialog = &dialog;
            dialog.setObjectName(QString::fromLatin1(PreferencesDialogName));
            dialog.resize(DialogSize);

            parts.m_pageTree = new QTreeWidget(&dialog);
            parts.m_pageTree->setObjectName(QStringLiteral("pageTree"));
            parts.m_pageTree->setColumnCount(1);
            parts.m_pageTree->setHeaderHidden(true);
            parts.m_pageTree->setRootIsDecorated(false);
            parts.m_pageTree->setGeometry(0, 0, TreeSize.width(), TreeSize.height());

            parts.m_editor = new AzToolsFramework::ReflectedPropertyEditor(&dialog);
            parts.m_editor->setObjectName(QStringLiteral("propertyEditor"));
            parts.m_editor->setFrameShape(QFrame::StyledPanel);
            parts.m_editor->setFrameShadow(QFrame::Raised);
            parts.m_editor->setGeometry(TreeSize.width(), 0, EditorSize.width(), EditorSize.height());

            parts.m_groupRow = makeRow(*parts.m_editor, "General Settings");
            parts.m_childRow = makeRow(*parts.m_editor, "Startup");
            parts.m_childRow->SetParentRow(parts.m_groupRow);
            parts.m_groupRow->AddedChild(parts.m_childRow);
            parts.m_leafRow = makeRow(*parts.m_editor, "Auto backup");

            // What a layout of the dialog would do with the box of the dialog's rule: a layout of its own, which holds one widget of
            // a fixed size (the rows and the lists above are placed by their geometry)
            parts.m_probe = new QWidget(&dialog);
            parts.m_probe->setFixedSize(ProbeSize);
            auto* layout = new QVBoxLayout(&dialog);
            layout->addWidget(parts.m_probe);
            return parts;
        }

        //! What a text element gave a widget: the size and weight of its font and the colour of its text in each group
        struct TextFacts
        {
            int m_pixelSize = 0;
            int m_weight = 0;
            QString m_family;
            QRgb m_active = 0;
            QRgb m_inactive = 0;
            QRgb m_disabled = 0;
        };

        TextFacts textFactsOf(const QWidget& widget)
        {
            TextFacts facts;
            facts.m_pixelSize = widget.font().pixelSize();     // the size asked for: no font need be installed
            facts.m_weight = widget.font().weight();
            facts.m_family = widget.font().family();
            facts.m_active = widget.palette().color(QPalette::Active, QPalette::WindowText).rgba();
            facts.m_inactive = widget.palette().color(QPalette::Inactive, QPalette::WindowText).rgba();
            facts.m_disabled = widget.palette().color(QPalette::Disabled, QPalette::WindowText).rgba();
            return facts;
        }

        //! The size an item of a view asks for (CT_ItemViewItem), for an item with a text, and with an icon as the items of the list of
        //! pages have (CreateImages), as a delegate asks it
        QSize itemSizeOf(const QAbstractItemView& view, const QString& text, bool withIcon = false)
        {
            QStyleOptionViewItem option;
            option.initFrom(&view);
            option.widget = &view;
            option.features |= QStyleOptionViewItem::HasDisplay;
            option.text = text;
            option.font = view.font();
            option.fontMetrics = view.fontMetrics();
            if (withIcon)
            {
                const int iconSize = view.style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, &view);
                QPixmap pixmap(iconSize, iconSize);
                pixmap.fill(Qt::white);
                option.features |= QStyleOptionViewItem::HasDecoration;
                option.icon = QIcon(pixmap);
                option.decorationSize = QSize(iconSize, iconSize);
            }
            return view.style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(), &view);
        }

        //! The base style of the legacy twin of a dialog's layout: Fusion, but answering the margins and the spacing a layout takes from
        //! its widget's style (PM_Layout*) as the base style of the legacy GUI does. The sheet style has no case for them
        //! (QStyleSheetStyle::pixelMetric hands them to its base style) and the application's chain is QStyleSheetStyle ->
        //! AzQtComponents::Style -> native base (StyleManager.cpp:346-351), whose Style answers 5 and 3 in every rung
        //! (Style.cpp:2869-2877); a plain Fusion base would answer Qt's own 11 for a window and 6 (QCommonStyle). Everything else is
        //! Fusion's, as in the other legacy twins.
        class LegacyLayoutBaseStyle
            : public QProxyStyle
        {
        public:
            explicit LegacyLayoutBaseStyle(const QStyle& applicationStyle)
                : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
                , m_applicationStyle(applicationStyle)
            {
            }

            int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override
            {
                switch (metric)
                {
                    case PM_LayoutLeftMargin:
                    case PM_LayoutTopMargin:
                    case PM_LayoutRightMargin:
                    case PM_LayoutBottomMargin:
                    case PM_LayoutHorizontalSpacing:
                    case PM_LayoutVerticalSpacing:
                        return m_applicationStyle.pixelMetric(metric, option, widget);
                    default:
                        return QProxyStyle::pixelMetric(metric, option, widget);
                }
            }

        private:
            const QStyle& m_applicationStyle;
        };

        class EditorDialogsFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The test application carries StyleManager's legacy sheets: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());
                m_applicationFont = QApplication::font();

                // The flattened GUI's branches of Style (CE_ShapedFrame, SE_ShapedFrameContents) and its elements read the rung
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_legacyLayoutBase = AZStd::make_unique<LegacyLayoutBaseStyle>(*m_style);

                // Once for the process, as the flattened GUI runs the registrar of a sheet once: the Global elements of the property
                // rows first (their rows compete with the dialog's), then the dialog's sheet
                static const bool registered = (AzQtComponents::PropertyEditorElements::registerGlobal(),
                    EditorFlatSheet::RegisterPreferencesDialogElements(), true);
                ASSERT_TRUE(registered);
            }

            void TearDown() override
            {
                m_legacyLayoutBase.reset();         // it holds the flat style
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                QApplication::setFont(m_applicationFont);       // without a class name: the class fonts of the flat run are cleared
                qApp->setStyleSheet(m_applicationSheet);
                LeakDetectionFixture::TearDown();
            }

            //! A style on a widget and on each below it: a style set on a widget reaches no child
            static void useStyle(QWidget& root, QStyle* style)
            {
                root.setStyle(style);
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(style);
                }
            }

            //! The flat way: the style, the class fonts the flattened GUI installs, and the polish it runs when a widget is shown,
            //! which resolves the elements
            void flatten(QWidget& root)
            {
                AzQtComponents::TextElement::installClassFonts();
                useStyle(root, m_style.get());
                root.ensurePolished();
            }

            static const char* boxOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            static const char* itemsOf(const QWidget& widget)
            {
                const AzQtComponents::ItemViewElement* element = AzQtComponents::ItemViewElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            //! The Editor Preferences dialog as the legacy GUI styled it: Qt's style sheet style over Fusion, the sheet the dialog sets
            //! on itself, and the application's rules above it (a widget's own sheet outranks the application's whatever the
            //! specificity of their rules). The callback gets the polished parts. The base style is Fusion unless a twin asks for another.
            void withLegacyPreferences(const AZStd::function<void(DialogParts&)>& use, QStyle* baseStyle = nullptr)
            {
                {
                    QDialog dialog;
                    DialogParts parts = buildDialog(dialog);
                    useStyle(dialog, baseStyle ? baseStyle : m_fusion.get());
                    qApp->setStyleSheet(QString::fromLatin1(UniversalRule) + QString::fromLatin1(DialogRule)
                        + QString::fromLatin1(ViewFillRules) + QString::fromLatin1(ViewRule) + QString::fromLatin1(ItemRules)
                        + QString::fromLatin1(ScrollAreaRule) + QString::fromLatin1(LabelRule) + QString::fromLatin1(RowNameRule));
                    dialog.setStyleSheet(QString::fromLatin1(PreferencesSheet));
                    dialog.ensurePolished();
                    use(parts);
                }
                qApp->setStyleSheet(QString());
            }

            //! The same dialog in the flattened GUI: no sheet, the elements
            void withFlatPreferences(const AZStd::function<void(DialogParts&)>& use)
            {
                QDialog dialog;
                DialogParts parts = buildDialog(dialog);
                flatten(dialog);
                use(parts);
            }

            QString m_applicationSheet;
            QFont m_applicationFont;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
            AZStd::unique_ptr<LegacyLayoutBaseStyle> m_legacyLayoutBase;
        };
    } // namespace EditorDialogsFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using EditorDialogsFlatSheetTestsInternal::EditorDialogsFlatSheetTests;
    namespace DialogsFlat = EditorDialogsFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution: the Editor Preferences dialog
    // ----------------------------------------------------------------------------------

    TEST_F(EditorDialogsFlatSheetTests, Registry_ThePreferencesDialogAndWhatTheSheetReachesInItHaveTheElements)
    {
        QDialog dialog;
        DialogsFlat::DialogParts parts = DialogsFlat::buildDialog(dialog);
        auto* metricsLabel = parts.m_groupRow->GetNameLabel()->findChild<QLabel*>();
        ASSERT_TRUE(metricsLabel != nullptr);                  // the hidden label an ElidingLabel measures with, of the same name
        flatten(dialog);

        EXPECT_STREQ(boxOf(dialog), DialogsFlat::DialogElementName);
        EXPECT_TRUE(dialog.testAttribute(Qt::WA_StyledBackground));         // the fill is drawn at PE_Widget
        EXPECT_STREQ(boxOf(*parts.m_pageTree), DialogsFlat::PageTreeElementName);
        EXPECT_STREQ(itemsOf(*parts.m_pageTree), DialogsFlat::PageTreeItemsElementName);
        EXPECT_STREQ(boxOf(*parts.m_editor), DialogsFlat::PropertyEditorElementName);
        EXPECT_STREQ(boxOf(*parts.m_groupRow), DialogsFlat::GroupRowElementName);

        // The name labels, and the labels they measure with, of every row: the dialog's box over the Global one of the rows
        for (QWidget* label : { static_cast<QWidget*>(parts.m_groupRow->GetNameLabel()), static_cast<QWidget*>(metricsLabel),
                                static_cast<QWidget*>(parts.m_childRow->GetNameLabel()),
                                static_cast<QWidget*>(parts.m_leafRow->GetNameLabel()) })
        {
            EXPECT_STREQ(boxOf(*label), DialogsFlat::RowNameElementName) << label->metaObject()->className();
        }
    }

    TEST_F(EditorDialogsFlatSheetTests, Registry_TheSameNamesOutsideThePreferencesDialogAndTheOtherWidgetsInsideItHaveNoneOfThem)
    {
        // The same widgets in a dialog of another name
        QDialog elsewhere;
        DialogsFlat::DialogParts stray = DialogsFlat::buildDialog(elsewhere);
        elsewhere.setObjectName(QStringLiteral("SomeOtherDialog"));

        // In the preferences dialog: a tree, a property editor and a frame of the names the sheet asks for but not of the classes, a
        // label of another name in a row, and a label of the name outside any row
        QDialog dialog;
        DialogsFlat::DialogParts parts = DialogsFlat::buildDialog(dialog);
        auto* otherTree = new QTreeWidget(&dialog);
        otherTree->setObjectName(QStringLiteral("otherTree"));
        auto* plainFrame = new QFrame(&dialog);                              // named propertyEditor, but not a ReflectedPropertyEditor
        plainFrame->setObjectName(QStringLiteral("propertyEditor"));
        auto* otherEditor = new AzToolsFramework::ReflectedPropertyEditor(&dialog);
        otherEditor->setObjectName(QStringLiteral("otherEditor"));
        auto* valueLabel = new QLabel(QStringLiteral("Value"), parts.m_groupRow);
        valueLabel->setObjectName(QStringLiteral("Value"));
        auto* looseName = new QLabel(QStringLiteral("Name"), &dialog);       // named Name, but in no row
        looseName->setObjectName(QStringLiteral("Name"));

        flatten(elsewhere);
        flatten(dialog);

        EXPECT_STRNE(boxOf(elsewhere), DialogsFlat::DialogElementName);
        EXPECT_STRNE(boxOf(*stray.m_pageTree), DialogsFlat::PageTreeElementName);
        EXPECT_STRNE(itemsOf(*stray.m_pageTree), DialogsFlat::PageTreeItemsElementName);
        EXPECT_STRNE(boxOf(*stray.m_editor), DialogsFlat::PropertyEditorElementName);
        EXPECT_STRNE(boxOf(*stray.m_groupRow), DialogsFlat::GroupRowElementName);
        // The Global element of the row's name is what a row outside the dialog has
        EXPECT_STREQ(boxOf(*stray.m_groupRow->GetNameLabel()), DialogsFlat::GlobalRowNameElementName);

        EXPECT_STRNE(boxOf(*otherTree), DialogsFlat::PageTreeElementName);
        EXPECT_STRNE(itemsOf(*otherTree), DialogsFlat::PageTreeItemsElementName);
        EXPECT_STRNE(boxOf(*plainFrame), DialogsFlat::PropertyEditorElementName);
        EXPECT_STRNE(boxOf(*otherEditor), DialogsFlat::PropertyEditorElementName);
        EXPECT_STRNE(boxOf(*valueLabel), DialogsFlat::RowNameElementName);
        EXPECT_STRNE(boxOf(*looseName), DialogsFlat::RowNameElementName);
    }

    TEST_F(EditorDialogsFlatSheetTests, Registry_ARowIsAGroupOnlyWhereBothPropertiesOfTheSelectorHold_AndNoRuleReachesWhatNoWidgetHas)
    {
        QDialog dialog;
        DialogsFlat::DialogParts parts = DialogsFlat::buildDialog(dialog);
        flatten(dialog);

        // The properties as the selector reads them
        EXPECT_TRUE(parts.m_groupRow->IsTopLevel());
        EXPECT_TRUE(parts.m_groupRow->HasChildRows());
        EXPECT_FALSE(parts.m_childRow->IsTopLevel());                        // its parent has a name
        EXPECT_TRUE(parts.m_leafRow->IsTopLevel());
        EXPECT_FALSE(parts.m_leafRow->HasChildRows());

        // Only the row with both has the group's box and its bold name
        EXPECT_STREQ(boxOf(*parts.m_groupRow), DialogsFlat::GroupRowElementName);
        EXPECT_STRNE(boxOf(*parts.m_childRow), DialogsFlat::GroupRowElementName);
        EXPECT_STRNE(boxOf(*parts.m_leafRow), DialogsFlat::GroupRowElementName);
        EXPECT_EQ(DialogsFlat::textFactsOf(*parts.m_groupRow->GetNameLabel()).m_weight, 700);
        EXPECT_EQ(DialogsFlat::textFactsOf(*parts.m_childRow->GetNameLabel()).m_weight, 400);
        EXPECT_EQ(DialogsFlat::textFactsOf(*parts.m_leafRow->GetNameLabel()).m_weight, 400);

        // `[HasParent="false"]` (:45-50) reaches nothing: no row has the property, so no name is white (its colour is #eeeeee in each)
        EXPECT_FALSE(parts.m_leafRow->property("HasParent").isValid());
        EXPECT_FALSE(parts.m_groupRow->property("HasParent").isValid());
        EXPECT_EQ(DialogsFlat::textFactsOf(*parts.m_leafRow->GetNameLabel()).m_active, 0xFFEEEEEEu);
    }

    // ----------------------------------------------------------------------------------
    // Pixels and boxes: the elements against the sheet, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(EditorDialogsFlatSheetTests, Pixels_TheDialogItsTreeItsEditorAndItsGroupRowPaintWhatTheSheetPaintedAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            QImage legacy[4];
            QImage flat[4];
            withLegacyPreferences([&legacy, ratio](DialogsFlat::DialogParts& parts)
                {
                    legacy[0] = DialogsFlat::paintBox(*parts.m_dialog, ratio);
                    legacy[1] = DialogsFlat::paintBox(*parts.m_pageTree, ratio);
                    legacy[2] = DialogsFlat::paintBox(*parts.m_editor, ratio);
                    legacy[3] = DialogsFlat::paintBox(*parts.m_groupRow, ratio);
                });
            withFlatPreferences([&flat, ratio](DialogsFlat::DialogParts& parts)
                {
                    flat[0] = DialogsFlat::paintBox(*parts.m_dialog, ratio);
                    flat[1] = DialogsFlat::paintBox(*parts.m_pageTree, ratio);
                    flat[2] = DialogsFlat::paintBox(*parts.m_editor, ratio);
                    flat[3] = DialogsFlat::paintBox(*parts.m_groupRow, ratio);
                });

            const char* const names[] = { "the dialog", "the list of pages", "the property editor", "the group row" };
            for (int part = 0; part < 4; ++part)
            {
                EXPECT_EQ(DialogsFlat::firstDifference(legacy[part], flat[part]), std::string()) << names[part] << " at ratio " << ratio;
            }
        }
    }

    TEST_F(EditorDialogsFlatSheetTests, Pixels_TheFillsAndLinesAreTheSheetsColoursToTheEdgesOfTheirBoxes)
    {
        withFlatPreferences([](DialogsFlat::DialogParts& parts)
            {
                const QImage dialog = DialogsFlat::paintBox(*parts.m_dialog, 1.5);
                EXPECT_EQ(dialog.pixel(0, 0) & 0xFFFFFF, 0x444444u);
                EXPECT_EQ(dialog.pixel(dialog.width() - 1, dialog.height() - 1) & 0xFFFFFF, 0x444444u);

                // The tree: the line (1px, 1.5 device pixels) around #333333; the editor: #222222 past its 5px and 2px of margin
                const QImage tree = DialogsFlat::paintBox(*parts.m_pageTree, 1.0);
                EXPECT_EQ(tree.pixel(0, 40) & 0xFFFFFF, 0x111111u);
                EXPECT_EQ(tree.pixel(50, 40) & 0xFFFFFF, 0x333333u);
                const QImage editor = DialogsFlat::paintBox(*parts.m_editor, 1.0);
                EXPECT_EQ(editor.pixel(2, 60) & 0xFFFFFF, DialogsFlat::ParentColor);         // inside the margin at the left
                EXPECT_EQ(editor.pixel(5, 60) & 0xFFFFFF, 0x111111u);                         // the line
                EXPECT_EQ(editor.pixel(6, 60) & 0xFFFFFF, 0x222222u);
                EXPECT_EQ(editor.pixel(editor.width() - 1, 60) & 0xFFFFFF, DialogsFlat::ParentColor);   // inside the margin at the right
            });
    }

    TEST_F(EditorDialogsFlatSheetTests, Box_TheContentsOfTheFramesAreThoseOfTheSheet)
    {
        QRect legacyRects[4];
        QRect flatRects[4];
        QMargins legacyMargins[4];
        QMargins flatMargins[4];
        const auto measure = [](DialogsFlat::DialogParts& parts, QRect (&rects)[4], QMargins (&margins)[4])
        {
            const QFrame* frames[] = { parts.m_pageTree, parts.m_editor, parts.m_groupRow, parts.m_groupRow->GetNameLabel() };
            for (int frame = 0; frame < 4; ++frame)
            {
                rects[frame] = DialogsFlat::frameContents(*frames[frame]);
                margins[frame] = frames[frame]->contentsMargins();
            }
        };
        withLegacyPreferences([&](DialogsFlat::DialogParts& parts) { measure(parts, legacyRects, legacyMargins); });
        withFlatPreferences([&](DialogsFlat::DialogParts& parts) { measure(parts, flatRects, flatMargins); });

        const char* const names[] = { "the list of pages", "the property editor", "the group row", "a name label" };
        for (int frame = 0; frame < 4; ++frame)
        {
            EXPECT_EQ(flatRects[frame], legacyRects[frame]) << names[frame];
            EXPECT_EQ(flatMargins[frame], legacyMargins[frame]) << names[frame];
        }

        // What they are: the line of 1px; that and the margin (5px, 2px); the padding above and the line, padding and margin below;
        // the margin and padding at the left of a name (the label is 80 x 16)
        EXPECT_EQ(flatRects[0], QRect(1, 1, DialogsFlat::TreeSize.width() - 2, DialogsFlat::TreeSize.height() - 2));
        EXPECT_EQ(flatRects[1], QRect(6, 1, DialogsFlat::EditorSize.width() - 6 - 3, DialogsFlat::EditorSize.height() - 2));
        EXPECT_EQ(flatRects[2], QRect(0, 4, DialogsFlat::RowSize.width(), DialogsFlat::RowSize.height() - 4 - 11));
        EXPECT_EQ(flatRects[3], QRect(9, 0, DialogsFlat::NameLabelSize.width() - 9, DialogsFlat::NameLabelSize.height()));
    }

    TEST_F(EditorDialogsFlatSheetTests, Box_TheBoxOfTheDialogsRuleLaysOutNothingInTheDialogInEitherGui)
    {
        // The rule's margin of 0 and its padding of -4px -5px -4px -5px make a box on a widget that is not a frame, and Qt's style sheet
        // style reads no box for such a widget (it has no layout case for it): the flat GUI carries the box on the dialog's element,
        // and this pins that neither GUI lets it move a layout, the contents or the hints of the dialog. The margins of the dialog's
        // layout come from the base style (PM_Layout*, which the sheet style does not read), so the legacy twin has the base style of
        // the legacy GUI for them (LegacyLayoutBaseStyle: AzQtComponents::Style's 5, not Fusion's 11)
        struct Layout
        {
            QRect m_probe;
            QRect m_contents;
            QMargins m_margins;
            QSize m_hint;
        };
        Layout legacy;
        Layout flat;
        const auto measure = [](DialogsFlat::DialogParts& parts, Layout& layout)
        {
            parts.m_dialog->layout()->activate();
            layout.m_probe = parts.m_probe->geometry();
            layout.m_contents = parts.m_dialog->contentsRect();
            layout.m_margins = parts.m_dialog->layout()->contentsMargins();
            layout.m_hint = parts.m_dialog->sizeHint();
        };
        withLegacyPreferences([&](DialogsFlat::DialogParts& parts) { measure(parts, legacy); }, m_legacyLayoutBase.get());
        withFlatPreferences([&](DialogsFlat::DialogParts& parts) { measure(parts, flat); });

        EXPECT_EQ(flat.m_probe, legacy.m_probe);
        EXPECT_EQ(flat.m_contents, legacy.m_contents);
        EXPECT_EQ(flat.m_margins, legacy.m_margins);
        EXPECT_EQ(flat.m_hint, legacy.m_hint);
        // No margin and no padding reach the contents: the box places only the fill
        EXPECT_EQ(flat.m_contents, QRect(QPoint(0, 0), DialogsFlat::DialogSize));
        EXPECT_EQ(flat.m_probe.size(), DialogsFlat::ProbeSize);
    }

    TEST_F(EditorDialogsFlatSheetTests, Text_ThePageTreeAndTheNamesHaveTheFontsAndTheColoursOfTheSheet)
    {
        DialogsFlat::TextFacts legacyTree;
        DialogsFlat::TextFacts legacyViewport;
        DialogsFlat::TextFacts legacyGroup;
        DialogsFlat::TextFacts legacyName;
        DialogsFlat::TextFacts legacyMetrics;
        withLegacyPreferences([&](DialogsFlat::DialogParts& parts)
            {
                legacyTree = DialogsFlat::textFactsOf(*parts.m_pageTree);
                legacyViewport = DialogsFlat::textFactsOf(*parts.m_pageTree->viewport());
                legacyGroup = DialogsFlat::textFactsOf(*parts.m_groupRow->GetNameLabel());
                legacyName = DialogsFlat::textFactsOf(*parts.m_leafRow->GetNameLabel());
                legacyMetrics = DialogsFlat::textFactsOf(*parts.m_groupRow->GetNameLabel()->findChild<QLabel*>());
            });

        DialogsFlat::TextFacts flatTree;
        DialogsFlat::TextFacts flatViewport;
        DialogsFlat::TextFacts flatGroup;
        DialogsFlat::TextFacts flatName;
        DialogsFlat::TextFacts flatMetrics;
        withFlatPreferences([&](DialogsFlat::DialogParts& parts)
            {
                flatTree = DialogsFlat::textFactsOf(*parts.m_pageTree);
                flatViewport = DialogsFlat::textFactsOf(*parts.m_pageTree->viewport());
                flatGroup = DialogsFlat::textFactsOf(*parts.m_groupRow->GetNameLabel());
                flatName = DialogsFlat::textFactsOf(*parts.m_leafRow->GetNameLabel());
                flatMetrics = DialogsFlat::textFactsOf(*parts.m_groupRow->GetNameLabel()->findChild<QLabel*>());
            });

        const auto expectSame = [](const DialogsFlat::TextFacts& flat, const DialogsFlat::TextFacts& legacy, const char* what)
        {
            EXPECT_EQ(flat.m_pixelSize, legacy.m_pixelSize) << what;
            EXPECT_EQ(flat.m_weight, legacy.m_weight) << what;
            EXPECT_EQ(flat.m_family.toStdString(), legacy.m_family.toStdString()) << what;
            EXPECT_EQ(flat.m_active, legacy.m_active) << what;
            EXPECT_EQ(flat.m_inactive, legacy.m_inactive) << what;
            EXPECT_EQ(flat.m_disabled, legacy.m_disabled) << what;
        };
        EXPECT_EQ(flatTree.m_pixelSize, legacyTree.m_pixelSize);            // the text of a page: 11px, whatever the view started from
        EXPECT_EQ(flatTree.m_weight, legacyTree.m_weight);
        EXPECT_EQ(flatTree.m_family.toStdString(), legacyTree.m_family.toStdString());

        // The delegate of the list (EditorPreferencesTreeWidgetItemDelegate::paint) draws the text of a page with drawText on the
        // painter it is given, which has the font of the VIEWPORT, not the option's: the sheet reached it through the container rule
        // (the font of the view), the flat GUI through the font the view's widgets inherit
        EXPECT_EQ(flatViewport.m_pixelSize, legacyViewport.m_pixelSize);
        EXPECT_EQ(flatViewport.m_weight, legacyViewport.m_weight);
        EXPECT_EQ(flatViewport.m_family.toStdString(), legacyViewport.m_family.toStdString());
        EXPECT_EQ(flatViewport.m_pixelSize, 11);
        expectSame(flatGroup, legacyGroup, "the name of a group row");
        expectSame(flatName, legacyName, "the name of another row");
        expectSame(flatMetrics, legacyMetrics, "the label an ElidingLabel measures with");

        EXPECT_EQ(flatTree.m_pixelSize, 11);
        EXPECT_EQ(flatGroup.m_pixelSize, 12);
        EXPECT_EQ(flatGroup.m_weight, 700);
        EXPECT_EQ(flatName.m_weight, 400);
        // #eeeeee in each colour group (the sheet's colour rule has no state)
        EXPECT_EQ(flatName.m_active, 0xFFEEEEEEu);
        EXPECT_EQ(flatName.m_inactive, 0xFFEEEEEEu);
        EXPECT_EQ(flatName.m_disabled, 0xFFEEEEEEu);
    }

    TEST_F(EditorDialogsFlatSheetTests, Size_TheNameLabelsHaveTheLimitsAndTheHintsOfTheSheet)
    {
        struct Sizes
        {
            QSize m_min;
            QSize m_max;
            QSize m_hint;
            QSize m_minHint;
            QSize m_metricsHint;
        };
        Sizes legacy;
        Sizes flat;
        const auto measure = [](DialogsFlat::DialogParts& parts, Sizes& sizes)
        {
            QLabel* label = parts.m_groupRow->GetNameLabel();
            sizes.m_min = label->minimumSize();
            sizes.m_max = label->maximumSize();
            sizes.m_hint = label->sizeHint();
            sizes.m_minHint = label->minimumSizeHint();
            sizes.m_metricsHint = label->findChild<QLabel*>()->sizeHint();
        };
        withLegacyPreferences([&](DialogsFlat::DialogParts& parts) { measure(parts, legacy); });
        withFlatPreferences([&](DialogsFlat::DialogParts& parts) { measure(parts, flat); });

        EXPECT_EQ(flat.m_min, legacy.m_min);
        EXPECT_EQ(flat.m_max, legacy.m_max);
        // the margin and padding of the label count twice: for it and for the label it measures with
        EXPECT_EQ(flat.m_hint, legacy.m_hint);
        EXPECT_EQ(flat.m_minHint, legacy.m_minHint);
        EXPECT_EQ(flat.m_metricsHint, legacy.m_metricsHint);
        EXPECT_EQ(flat.m_min.height(), 16);
        EXPECT_EQ(flat.m_max.height(), 16);
    }

    TEST_F(EditorDialogsFlatSheetTests, Items_ThePagesOfTheTreeAreTheSizeOfTheSheetsItems)
    {
        QSize legacy;
        QSize flat;
        QSize legacyWithIcon;
        QSize flatWithIcon;
        const QString text = QStringLiteral("General");
        withLegacyPreferences([&](DialogsFlat::DialogParts& parts)
            {
                legacy = DialogsFlat::itemSizeOf(*parts.m_pageTree, text);
                legacyWithIcon = DialogsFlat::itemSizeOf(*parts.m_pageTree, text, true);
            });
        withFlatPreferences([&](DialogsFlat::DialogParts& parts)
            {
                flat = DialogsFlat::itemSizeOf(*parts.m_pageTree, text);
                flatWithIcon = DialogsFlat::itemSizeOf(*parts.m_pageTree, text, true);
            });

        EXPECT_EQ(flat, legacy);
        EXPECT_EQ(flatWithIcon, legacyWithIcon);                    // the items of the list have an icon each (CreateImages)
        // The rule's 24px of height and the margin above and below it (3px and 2px), the padding at the left (4px)
        EXPECT_EQ(flat.height(), 24 + 3 + 2);
        EXPECT_GT(flat.width(), 4);
    }
} // namespace UnitTest
