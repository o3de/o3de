/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <QApplication>
#include <QFrame>
#include <QLabel>
#include <QStringList>
#include <QWidget>

#include <string>
#include <utility>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The rows of the scene settings tool (PropertyRowWidget::Initialize, RefreshAttributesFromNode): a row below the row of a
    // scene setting sets `QFrame {background-color: #555555; color: white;}` on itself (C050), the row of a group in the display
    // divider style sets `QFrame {background-color: #333333; margin-top: 5px; border-top-*-radius: 2px;}` (C051). Each sheet reaches
    // the row and every QFrame below it (the labels too). The legacy twin is Qt's own style sheet style over AzQtComponents::Style
    // with the real application sheet and the sheets quoted; the flat twin is the flattened Style with the elements the rows
    // register, and no sheet. The rows are the real class, initialized on the nodes of a reflected group as a property editor
    // initializes them.
    // ----------------------------------------------------------------------------------

    namespace PropertyRowSceneSettingsFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        //! The test application carries StyleManager's legacy sheets, which the twins name their own of: none while a test runs
        class ApplicationSheetGuard
        {
        public:
            ApplicationSheetGuard()
                : m_sheet(qApp->styleSheet())
            {
                qApp->setStyleSheet(QString());
            }

            ~ApplicationSheetGuard()
            {
                qApp->setStyleSheet(m_sheet);
            }

            ApplicationSheetGuard(const ApplicationSheetGuard&) = delete;
            ApplicationSheetGuard& operator=(const ApplicationSheetGuard&) = delete;

        private:
            QString m_sheet;
        };

        // ------------------------------------------------------------------------------
        // The sheets, quoted as the source has them (PropertyRowWidget.cpp:64 and :65-66)
        // ------------------------------------------------------------------------------

        constexpr const char* SettingSheet = "QFrame {background-color: #555555; color: white;}";
        constexpr const char* DividerSheet =
            "QFrame {background-color: #333333; margin-top: 5px; border-top-right-radius: 2px; border-top-left-radius: 2px;}";

        constexpr QSize RowBoxSize(150, 40);            // the box a row is given in the twins
        constexpr QSize NameBoxSize(80, 16);            // the box of the name of a row
        constexpr QSize DefaultBoxSize(60, 16);         // the box of the text a row shows in place of a control

        // ------------------------------------------------------------------------------
        // A group as the scene settings reflect it: the display divider category on the class, two settings in it
        // ------------------------------------------------------------------------------

        struct SceneGroupSample
        {
            AZ_TYPE_INFO(SceneGroupSample, "{3C3E86A7-8F4E-4D6B-A9D3-7B0E2C5F1A64}");

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<SceneGroupSample>()
                    ->Version(1)
                    ->Field("enabled", &SceneGroupSample::m_enabled)
                    ->Field("scale", &SceneGroupSample::m_scale);

                if (AZ::EditContext* editContext = context.GetEditContext())
                {
                    editContext->Class<SceneGroupSample>("Scene group", "A group of the scene settings")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::CategoryStyle, "display divider")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SceneGroupSample::m_enabled, "Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::ValueText, "On")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SceneGroupSample::m_scale, "Scale", "");
                }
            }

            bool m_enabled = true;
            float m_scale = 1.0f;
        };

        //! The context, the group and the hierarchy of its nodes: they outlive the rows built on them
        struct SceneGroupData
        {
            SceneGroupData()
            {
                m_context.CreateEditContext();
                SceneGroupSample::Reflect(m_context);
                m_hierarchy.AddRootInstance(&m_group, AZ::AzTypeInfo<SceneGroupSample>::Uuid());
                m_hierarchy.Build(&m_context, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
            }

            AzToolsFramework::InstanceDataNode* groupNode()
            {
                return m_hierarchy.GetRootNode();
            }

            AzToolsFramework::InstanceDataNode* settingNode()
            {
                return &m_hierarchy.GetChildren().front();
            }

            AZ::SerializeContext m_context;
            SceneGroupSample m_group;
            AzToolsFramework::InstanceDataHierarchy m_hierarchy;
        };

        //! A row below a parent widget; the property a property editor gives it keeps PropertyRowWidget::SetOverridden from repolishing
        //! the row through the StyleManager, which these tests have not
        AzToolsFramework::PropertyRowWidget* newRow(QWidget& parent)
        {
            auto* row = new AzToolsFramework::PropertyRowWidget(&parent);
            row->setProperty("IsOverridden", false);
            row->setGeometry(0, 0, RowBoxSize.width(), RowBoxSize.height());
            return row;
        }

        //! The rows of the group and of its first setting, as a property editor builds them: the group is a row of no parent row, the
        //! setting a row below it. The labels are sized as the rows lay them out.
        struct SceneRows
        {
            AzToolsFramework::PropertyRowWidget* m_group = nullptr;
            AzToolsFramework::PropertyRowWidget* m_setting = nullptr;
        };

        SceneRows buildSceneRows(QWidget& parent, SceneGroupData& data)
        {
            SceneRows rows;
            rows.m_group = newRow(parent);
            rows.m_group->Initialize(nullptr, data.groupNode(), 0);
            rows.m_group->GetNameLabel()->setGeometry(0, 0, NameBoxSize.width(), NameBoxSize.height());

            rows.m_setting = newRow(parent);
            rows.m_setting->Initialize(rows.m_group, data.settingNode(), 1);
            rows.m_setting->GetNameLabel()->setGeometry(0, 0, NameBoxSize.width(), NameBoxSize.height());
            rows.m_group->AddedChild(rows.m_setting);
            return rows;
        }

        //! The text a row shows in place of a control, as the row shows it (UpdateDefaultLabel): the row of the setting has it
        QLabel* defaultLabelOf(AzToolsFramework::PropertyRowWidget* row)
        {
            auto* label = row->findChild<QLabel*>(QStringLiteral("DefaultLabel"));
            if (label)
            {
                label->setText(QStringLiteral("Default"));
                label->setGeometry(0, 0, DefaultBoxSize.width(), DefaultBoxSize.height());
                label->show();
            }
            return label;
        }

        // ------------------------------------------------------------------------------
        // The rows of the twins
        // ------------------------------------------------------------------------------

        //! The roles a sheet's `color` decides. The background roles are what Qt's style sheet style derives from a `background-color`
        //! (the fill of a sheet is painted by the style, not read from the palette), so they are no fact of the flattened GUI
        std::vector<QPalette::ColorRole> textRoles()
        {
            return { QPalette::WindowText, QPalette::Text, QPalette::ButtonText };
        }

        constexpr unsigned BoxFacts = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels;
        constexpr unsigned LabelFacts = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels;

        enum class Which
        {
            Group,
            Setting,
        };

        /*! The link of the chain that is a row: it builds the group and its setting, and is the one of them `which` names */
        Twins::Link rowLink(SceneGroupData& data, Which which)
        {
            return { .m_name = "row",
                .m_make = [&data, which](QWidget* parent) -> QWidget*
                {
                    const SceneRows rows = buildSceneRows(*parent, data);
                    return which == Which::Group ? rows.m_group : rows.m_setting;
                } };
        }

        const char* sheetOf(Which which)
        {
            return which == Which::Group ? DividerSheet : SettingSheet;
        }

        const char* citeOf(Which which)
        {
            return which == Which::Group ? "PropertyRowWidget.cpp:66" : "PropertyRowWidget.cpp:64";
        }

        //! The row itself: it carries the sheet
        Twins::Row rowRow(SceneGroupData& data, Which which, const char* site)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = [&data, which](QWidget* parent) -> QWidget*
                {
                    const SceneRows rows = buildSceneRows(*parent, data);
                    return which == Which::Group ? rows.m_group : rows.m_setting;
                },
                .m_sheets = { { citeOf(which), Twins::SetOn::Widget, sheetOf(which) } },
                .m_realApplicationSheet = true,
                .m_compare = BoxFacts,
                .m_roles = textRoles(),
                .m_size = RowBoxSize,
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the rows set their sheet themselves: the twin without the quoted sheets has it too
            };
        }

        //! A widget below the row, which the sheet of the row reaches: the row is a link of the chain and the sheet is set on it
        Twins::Row belowRow(
            SceneGroupData& data, Which which, const char* site, Twins::WidgetFactory make, const QSize& size, unsigned compare)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = std::move(make),
                .m_chain = { rowLink(data, which) },
                .m_sheets = { { citeOf(which), Twins::SetOn::Ancestor, sheetOf(which), 0 } },
                .m_realApplicationSheet = true,
                .m_compare = compare,
                .m_roles = textRoles(),
                .m_size = size,
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the rows set their sheet themselves: the twin without the quoted sheets has it too
            };
        }

        QWidget* nameLabelOf(QWidget* parent)
        {
            return static_cast<AzToolsFramework::PropertyRowWidget*>(parent)->GetNameLabel();
        }

        QWidget* defaultLabelBelow(QWidget* parent)
        {
            return defaultLabelOf(static_cast<AzToolsFramework::PropertyRowWidget*>(parent));
        }

        //! A frame of a handler widget (the vector widget's header frame, a StyledPanel) and a row of a nested editor, which the sheet of
        //! the row below which they lie reaches
        QWidget* panelBelow(QWidget* parent)
        {
            auto* panel = new QFrame(parent);
            panel->setObjectName(QStringLiteral("frame"));
            panel->setFrameShape(QFrame::StyledPanel);
            panel->setFrameShadow(QFrame::Raised);
            panel->setGeometry(0, 0, 60, 24);
            return panel;
        }

        QWidget* nestedRowBelow(QWidget* parent)
        {
            return newRow(*parent);
        }

        //! The label of the first coordinate of a vector in the handler widget of a row (VectorInput: a grid of vector elements, each a
        //! label and a number field). Its application rule fills it with the colour of its coordinate (VectorInput.qss:37-40); the
        //! sheet of the row, deeper, fills it with its own colour.
        QWidget* vectorLabelBelow(QWidget* parent)
        {
            auto* vector = new AzQtComponents::VectorInput(parent, 3);
            vector->setGeometry(0, 0, RowBoxSize.width(), 24);
            // The number fields take no focus: the watcher of the number fields asks the StyleManager when one is focused (the window
            // of a twin is activated and focuses the first field), and this process has none. The label is all that is compared.
            for (QWidget* each : vector->findChildren<QWidget*>())
            {
                each->setFocusPolicy(Qt::NoFocus);
            }
            return vector->getElements()[0]->getLabelWidget();
        }

        //! The editor of a set of rows in the handler widget of a row (a QFrame of a class of its own), as the editors of a component
        //! are set up without scroll bars
        QWidget* nestedEditorBelow(QWidget* parent)
        {
            auto* editor = new AzToolsFramework::ReflectedPropertyEditor(parent);
            editor->Setup(nullptr, nullptr, false, NameBoxSize.width());
            editor->setGeometry(0, 0, RowBoxSize.width(), RowBoxSize.height());
            return editor;
        }

        class PropertyRowSceneSettingsFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                m_data = AZStd::make_unique<SceneGroupData>();
            }

            void TearDown() override
            {
                m_data.reset();
                LeakDetectionFixture::TearDown();
            }

            ApplicationSheetGuard m_applicationSheet;
            Twins::Context m_context;
            AZStd::unique_ptr<SceneGroupData> m_data;
        };
    } // namespace PropertyRowSceneSettingsFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in
    using PropertyRowSceneSettingsFlatSheetTestsInternal::PropertyRowSceneSettingsFlatSheetTests;
    namespace SceneRow = PropertyRowSceneSettingsFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // C050: the row of a scene setting
    // ----------------------------------------------------------------------------------

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_HasTheFillOfItsSheet)
    {
        EXPECT_TRUE(SceneRow::Twins::check(
            SceneRow::rowRow(*m_data, SceneRow::Which::Setting, "C050 the row of a scene setting"), m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_NameLabelHasTheFillOfTheRowsSheet)
    {
        EXPECT_TRUE(SceneRow::Twins::check(
            SceneRow::belowRow(*m_data, SceneRow::Which::Setting, "C050 the name of a scene setting", &SceneRow::nameLabelOf,
                SceneRow::NameBoxSize, SceneRow::LabelFacts),
            m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_NameLabelDisabledHasTheTextOfTheRowsSheet)
    {
        SceneRow::Twins::Row row = SceneRow::belowRow(*m_data, SceneRow::Which::Setting, "C050 the name of a scene setting, disabled",
            &SceneRow::nameLabelOf, SceneRow::NameBoxSize, SceneRow::LabelFacts);
        row.m_states = { SceneRow::Twins::States::disabled() };
        EXPECT_TRUE(SceneRow::Twins::check(row, m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_DefaultLabelHasTheFillOfTheRowsSheet)
    {
        EXPECT_TRUE(SceneRow::Twins::check(
            SceneRow::belowRow(*m_data, SceneRow::Which::Setting, "C050 the text of a scene setting without a control",
                &SceneRow::defaultLabelBelow, SceneRow::DefaultBoxSize, SceneRow::LabelFacts),
            m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_FrameOfAHandlerWidgetHasTheFillOfTheRowsSheet)
    {
        EXPECT_TRUE(SceneRow::Twins::check(
            SceneRow::belowRow(*m_data, SceneRow::Which::Setting, "C050 a frame in the handler widget of a scene setting",
                &SceneRow::panelBelow, QSize(60, 24), SceneRow::BoxFacts),
            m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_RowOfANestedEditorHasTheFillOfTheRowsSheet)
    {
        // The row has no content: the fill is all it draws, in one colour
        SceneRow::Twins::Row row = SceneRow::belowRow(*m_data, SceneRow::Which::Setting,
            "C050 a row of an editor in the handler widget of a scene setting", &SceneRow::nestedRowBelow, SceneRow::RowBoxSize,
            SceneRow::BoxFacts);
        row.m_mayDrawNothing = true;
        EXPECT_TRUE(SceneRow::Twins::check(row, m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_CoordinateLabelOfAVectorHasTheFillOfTheRowsSheet)
    {
        // The row's fill (#555555) is in place of the colour of the coordinate in the legacy GUI; the label keeps its box. The flattened
        // Style paints the colour of the coordinate when no element takes the label, which the pictures show.
        SceneRow::Twins::Row row = SceneRow::belowRow(*m_data, SceneRow::Which::Setting,
            "C050 the label of a coordinate of a vector in the handler widget of a scene setting", &SceneRow::vectorLabelBelow, QSize(),
            SceneRow::LabelFacts);
        EXPECT_TRUE(SceneRow::Twins::check(row, m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, SettingRow_EditorOfNestedRowsHasTheFillOfTheRowsSheet)
    {
        // The editor is a frame of a class of its own: the fill is all it draws, in one colour
        SceneRow::Twins::Row row = SceneRow::belowRow(*m_data, SceneRow::Which::Setting,
            "C050 an editor of rows in the handler widget of a scene setting", &SceneRow::nestedEditorBelow, SceneRow::RowBoxSize,
            SceneRow::BoxFacts);
        row.m_mayDrawNothing = true;
        EXPECT_TRUE(SceneRow::Twins::check(row, m_context));
    }

    // ----------------------------------------------------------------------------------
    // C051: the row of a group in the display divider style
    // ----------------------------------------------------------------------------------

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, DividerRow_HasTheBoxAndTheFillOfItsSheet)
    {
        EXPECT_TRUE(SceneRow::Twins::check(
            SceneRow::rowRow(*m_data, SceneRow::Which::Group, "C051 the row of a group in the display divider style"), m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, DividerRow_NameLabelHasTheMarginFillAndCornersOfTheRowsSheet)
    {
        EXPECT_TRUE(SceneRow::Twins::check(
            SceneRow::belowRow(*m_data, SceneRow::Which::Group, "C051 the name of a group in the display divider style",
                &SceneRow::nameLabelOf, SceneRow::NameBoxSize, SceneRow::LabelFacts),
            m_context));
    }

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, DividerRow_DefaultLabelHasTheMarginFillAndCornersOfTheRowsSheet)
    {
        EXPECT_TRUE(SceneRow::Twins::check(
            SceneRow::belowRow(*m_data, SceneRow::Which::Group, "C051 the text of a group in the display divider style",
                &SceneRow::defaultLabelBelow, SceneRow::DefaultBoxSize, SceneRow::LabelFacts),
            m_context));
    }

    // ----------------------------------------------------------------------------------
    // The contents margins that the code sets on the rows (setContentsMargins) and the box of the sheet: a QFrame keeps its margins
    // as the margins it set plus the box (QFrame::setFrameRect), and the divider row sets its margins before its sheet, the setting
    // row after it. Both GUIs end with the same margins; the divider row's top is the 5px of the sheet's margin.
    // ----------------------------------------------------------------------------------

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, Rows_EndWithTheContentsMarginsOfTheLegacyGui)
    {
        const auto marginsAt = [this](AzQtComponents::GuiPerfFloor::Rung rung)
        {
            SceneRow::Twins::RungScope scope(rung);
            QWidget host;
            const SceneRow::SceneRows rows = SceneRow::buildSceneRows(host, *m_data);
            if (rung == AzQtComponents::GuiPerfFloor::Rung::Flat)
            {
                host.setStyle(m_context.flatStyle());
                for (QWidget* each : host.findChildren<QWidget*>())
                {
                    each->setStyle(m_context.flatStyle());
                }
            }
            host.setAttribute(Qt::WA_DontShowOnScreen);
            host.show();
            for (QWidget* each : host.findChildren<QWidget*>())
            {
                each->ensurePolished();
            }
            return std::make_pair(rows.m_group->contentsMargins(), rows.m_setting->contentsMargins());
        };

        for (const auto rung : { AzQtComponents::GuiPerfFloor::Rung::Qss, AzQtComponents::GuiPerfFloor::Rung::Flat })
        {
            const auto margins = marginsAt(rung);
            EXPECT_EQ(margins.first, QMargins(8, 5, 4, 0)) << "the divider row, rung " << static_cast<int>(rung);
            EXPECT_EQ(margins.second, QMargins(0, 0, 8, 0)) << "the setting row, rung " << static_cast<int>(rung);
        }
    }

    // ----------------------------------------------------------------------------------
    // The flattened GUI sets no sheet on a row
    // ----------------------------------------------------------------------------------

    TEST_F(PropertyRowSceneSettingsFlatSheetTests, Rows_SetSheetsOnlyInTheLegacyGui)
    {
        const auto build = [this]
        {
            QWidget host;
            host.resize(300, 200);
            SceneRow::buildSceneRows(host, *m_data);
            host.setAttribute(Qt::WA_DontShowOnScreen);
            host.show();
            for (QWidget* each : host.findChildren<QWidget*>())
            {
                each->ensurePolished();
            }
        };
        // the group's row and its setting's: one sheet each, on the PropertyRowWidget
        const QStringList legacy = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Qss, build);
        int rows = 0;
        for (const QString& line : legacy)
        {
            rows += AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("AzToolsFramework::PropertyRowWidget")) ? 1 : 0;
        }
        EXPECT_EQ(rows, 2) << legacy.join("\n").toStdString();
        const QStringList flat = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Flat, build);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }
} // namespace UnitTest
