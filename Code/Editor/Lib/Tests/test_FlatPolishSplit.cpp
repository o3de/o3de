/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include <EditorFlatSheet.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/GuiPerfCounters.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/IconElidingLabel.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserListView.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerFlatSheet.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyleFactory>
#include <QToolButton>
#include <QWidget>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // T.17, the split per class and per registrar (plan flatten_missing_styles; catalogue performance.md, M2).
    // The flattened Style's own polish time per polish, for widgets of each class the Entity Inspector polishes, below a chain
    // of plain widgets as deep as an inspector row sits in the Editor's main window. Measured with no rows, then again after each
    // registrar the Editor runs at startup, in the Editor's order: each step's delta is the cost of that registrar's rows for that
    // class. A measurement, not a check: disabled in the suites, run alone (the registries keep their rows for the process):
    //   EditorLib.Tests AzRunUnitTests --gtest_also_run_disabled_tests --gtest_filter=*FlatPolishSplit*
    // ----------------------------------------------------------------------------------

    namespace FlatPolishSplitInternal
    {
        using AzQtComponents::GuiPerfCounters;

        // ------------------------------------------------------------------------------
        // What is measured
        // ------------------------------------------------------------------------------

        constexpr int ChainDepth = 18;          //!< main window, docks, inspector, scroll area, viewport, cards, DPE, its surface...
        constexpr int WidgetsPerBatch = 300;
        constexpr int Batches = 7;              //!< the median batch is kept

        //! A widget class of the inspector, made below a parent
        struct Kind
        {
            const char* m_name;
            QWidget* (*m_make)(QWidget* parent);
        };

        const Kind Kinds[] = {
            { "QWidget", [](QWidget* parent) -> QWidget* { return new QWidget(parent); } },
            { "QFrame", [](QWidget* parent) -> QWidget* { return new QFrame(parent); } },
            { "QLabel", [](QWidget* parent) -> QWidget* { return new QLabel(QStringLiteral("Label"), parent); } },
            { "ElidingLabel",
              [](QWidget* parent) -> QWidget* { return new AzQtComponents::ElidingLabel(QStringLiteral("Label"), parent); } },
            { "QToolButton", [](QWidget* parent) -> QWidget* { return new QToolButton(parent); } },
            { "QPushButton", [](QWidget* parent) -> QWidget* { return new QPushButton(QStringLiteral("Button"), parent); } },
            { "QCheckBox", [](QWidget* parent) -> QWidget* { return new QCheckBox(parent); } },
            { "QLineEdit", [](QWidget* parent) -> QWidget* { return new QLineEdit(parent); } },
            { "QComboBox", [](QWidget* parent) -> QWidget* { return new QComboBox(parent); } },
            { "IconElidingLabel", [](QWidget* parent) -> QWidget* { return new AzQtComponents::IconElidingLabel(parent); } },
            { "DoubleSpinBox", [](QWidget* parent) -> QWidget* { return new AzQtComponents::DoubleSpinBox(parent); } },
            { "SliderDouble", [](QWidget* parent) -> QWidget* { return new AzQtComponents::SliderDouble(parent); } },
            { "DPERowWidget",
              [](QWidget* parent) -> QWidget*
              {
                  auto* row = new AzToolsFramework::DPERowWidget();
                  row->setParent(parent);
                  return row;
              } },
        };
        constexpr int KindCount = static_cast<int>(sizeof(Kinds) / sizeof(Kinds[0]));

        //! A registrar the Editor runs at startup, in the order it runs them: the Style's Global rows when the application is
        //! polished (Style.cpp, Style::polish(QApplication*)), then the sheets and the widgets that register their own
        struct Step
        {
            const char* m_name;
            void (*m_register)();
        };

        const Step Steps[] = {
            { "no rows", nullptr },
            { "ControlElements", &AzQtComponents::ControlElements::registerGlobal },
            { "ColorPickerElements", &AzQtComponents::ColorPickerElements::registerGlobal },
            { "PropertyEditorElements", &AzQtComponents::PropertyEditorElements::registerGlobal },
            { "ComboBox", &AzQtComponents::ComboBox::registerFlatElements },
            { "EditorFlatSheet", &EditorFlatSheet::RegisterElements },
            { "EntityOutliner", &AzToolsFramework::EntityOutliner::RegisterFlatSheetElements },
            { "ViewportUiDisplay", &AzToolsFramework::ViewportUi::Internal::ViewportUiDisplay::RegisterFlatElements },
            { "EntityPropertyEditor", &AzToolsFramework::EntityPropertyEditor::RegisterFlatElements },
            { "AssetBrowserListView", &AzToolsFramework::AssetBrowser::AssetBrowserListView::RegisterFlatElements },
            { "AssetBrowserInspector", &AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget::RegisterFlatElements },
        };
        constexpr int StepCount = static_cast<int>(sizeof(Steps) / sizeof(Steps[0]));

        // ------------------------------------------------------------------------------
        // The measurement
        // ------------------------------------------------------------------------------

        //! The chain of plain widgets the measured widgets sit below: no name or class any matcher looks for, so every walk runs to
        //! the root, as it does for a row of the inspector
        QWidget* buildChain(QWidget& root)
        {
            QWidget* parent = &root;
            for (int depth = 0; depth < ChainDepth; ++depth)
            {
                auto* link = new QWidget(parent);
                link->setObjectName(QStringLiteral("chain%1").arg(depth));
                parent = link;
            }
            return parent;
        }

        //! The flattened Style's time per polish, in microseconds, for widgets of the kind below the parent: the median batch. Each
        //! widget and the widgets it made take the style, then the widget is polished as Qt polishes it (its children after it). The
        //! style is set inside the count: a widget its constructor polished already (ElidingLabel.cpp:82) is polished by setStyle.
        double microsecondsPerPolish(QStyle* style, QWidget* parent, const Kind& kind, double& polishesPerWidget)
        {
            std::vector<double> perBatch;
            for (int batch = 0; batch < Batches; ++batch)
            {
                std::vector<QWidget*> widgets;
                widgets.reserve(WidgetsPerBatch);
                for (int i = 0; i < WidgetsPerBatch; ++i)
                {
                    widgets.push_back(kind.m_make(parent));
                }

                const GuiPerfCounters::Counters before = GuiPerfCounters::counters();
                for (QWidget* widget : widgets)
                {
                    widget->setStyle(style);
                    for (QWidget* child : widget->findChildren<QWidget*>())
                    {
                        child->setStyle(style);
                    }
                    widget->ensurePolished();
                }
                const GuiPerfCounters::Counters after = GuiPerfCounters::counters();

                const quint64 polishes = after.m_polish - before.m_polish;
                EXPECT_GE(polishes, static_cast<quint64>(WidgetsPerBatch)) << kind.m_name;
                polishesPerWidget = static_cast<double>(polishes) / WidgetsPerBatch;
                const quint64 nanoseconds = after.m_polishNanoseconds - before.m_polishNanoseconds;
                perBatch.push_back(polishes ? static_cast<double>(nanoseconds) / 1000.0 / static_cast<double>(polishes) : 0.0);

                for (QWidget* widget : widgets)
                {
                    delete widget;
                }
            }
            std::sort(perBatch.begin(), perBatch.end());
            return perBatch[perBatch.size() / 2];
        }
    } // namespace FlatPolishSplitInternal

    // ----------------------------------------------------------------------------------
    // The test
    // ----------------------------------------------------------------------------------

    class FlatPolishSplitTests
        : public LeakDetectionFixture
    {
    };

    TEST_F(FlatPolishSplitTests, DISABLED_FlatPolishSplit_PerClassAndRegistrar)
    {
        using namespace FlatPolishSplitInternal;

        AzQtComponents::SheetTwins::RungScope flat(AzQtComponents::GuiPerfFloor::Rung::Flat);
        GuiPerfCounters::install(qApp);
        ASSERT_TRUE(GuiPerfCounters::isInstalled());

        auto style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
        double table[StepCount][KindCount] = {};
        double polishesPerWidget[KindCount] = {};
        {
            QWidget root;
            root.setObjectName(QStringLiteral("root"));
            QWidget* parent = buildChain(root);

            for (int step = 0; step < StepCount; ++step)
            {
                if (Steps[step].m_register)
                {
                    Steps[step].m_register();
                }
                for (int kind = 0; kind < KindCount; ++kind)
                {
                    table[step][kind] = microsecondsPerPolish(style.get(), parent, Kinds[kind], polishesPerWidget[kind]);
                    std::printf("FlatPolishSplit: step=%s class=%s us_per_polish=%.3f\n", Steps[step].m_name, Kinds[kind].m_name,
                        table[step][kind]);
                }
            }
        }

        // The table: each step's time, and its delta over the step before (the cost of that registrar's rows for the class)
        std::printf("\nFlatPolishSplit: us per polish after each step (delta over the step before)\n%-24s", "step");
        for (int kind = 0; kind < KindCount; ++kind)
        {
            std::printf(" %18s", Kinds[kind].m_name);
        }
        std::printf("\n");
        for (int step = 0; step < StepCount; ++step)
        {
            std::printf("%-24s", Steps[step].m_name);
            for (int kind = 0; kind < KindCount; ++kind)
            {
                const double delta = step ? table[step][kind] - table[step - 1][kind] : 0.0;
                std::printf(" %8.3f (%+7.3f)", table[step][kind], delta);
            }
            std::printf("\n");
        }
        std::printf("%-24s", "polishes per widget");
        for (int kind = 0; kind < KindCount; ++kind)
        {
            std::printf(" %18.1f", polishesPerWidget[kind]);
        }
        std::printf("\n");
        std::fflush(stdout);
    }
} // namespace UnitTest
