/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/tuple.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>
#include <UI/EntityPropertyEditorTestFixture.h>

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The inspector's lazy cards (ed_inspectorLazyCards): the cards past the view and collapsed cards hold their rows; the rows are
    // built top to bottom later. Once built, the inspector is the one a full build gives.
    // ----------------------------------------------------------------------------------

    namespace EntityPropertyEditorLazyCardsTestsInternal
    {
        //! What the DPE's Tab stops on (DocumentPropertyEditor.cpp IsInputWidget), not counting a widget whose focus goes to another
        bool IsTabStop(const QWidget* widget)
        {
            const bool input = qobject_cast<const QAbstractSpinBox*>(widget) || qobject_cast<const QComboBox*>(widget) ||
                qobject_cast<const QCheckBox*>(widget) || qobject_cast<const QLineEdit*>(widget) || qobject_cast<const QPushButton*>(widget);
            return input && !widget->focusProxy();
        }

        //! One card as the user sees it once its rows are built
        struct CardRecord
        {
            QString m_title;
            QStringList m_labels;   //!< the labels of the card, top to bottom
            int m_height = 0;
            QStringList m_numbers;  //!< the texts of the card's spin boxes, top to bottom
        };

        //! The texts of the card's shown widgets of a type (a label, a spin box), top to bottom
        template<class Widget>
        QStringList ShownTexts(const QWidget* card)
        {
            QList<Widget*> widgets = card->findChildren<Widget*>();
            AZStd::sort(widgets.begin(), widgets.end(),
                [](const QWidget* a, const QWidget* b)
                {
                    return a->mapTo(a->window(), QPoint(0, 0)).y() < b->mapTo(b->window(), QPoint(0, 0)).y();
                });
            QStringList texts;
            for (const Widget* widget : widgets)
            {
                if (widget->isVisible())
                {
                    texts.push_back(widget->text());
                }
            }
            return texts;
        }
    } // namespace EntityPropertyEditorLazyCardsTestsInternal

    class EntityPropertyEditorLazyCardsTests : public EntityPropertyEditorTestFixture
    {
    protected:
        using CardRecord = EntityPropertyEditorLazyCardsTestsInternal::CardRecord;

        static constexpr int TestComponents = 6;

        void SetUpEditorFixtureImpl() override
        {
            EntityPropertyEditorTestFixture::SetUpEditorFixtureImpl();
            GetApplication()->RegisterComponentDescriptor(InspectorTallTestComponent::CreateDescriptor());

            // the transform and six tall cards: the inspector of a short window shows the first two
            m_entityId = CreateEntityOf<InspectorTallTestComponent>("LazyCardsEntity", TestComponents);
        }

        //! A new inspector of the given height, shown, with lazy cards on or off, showing the entity (its rebuild has just run)
        void Show(bool lazy, int windowHeight = 320)
        {
            SetCvar("ed_inspectorLazyCards", lazy);
            NewInspector(windowHeight);
            Select({ m_entityId });
        }

        //! The test components, in their order on the entity
        AZStd::vector<InspectorTallTestComponent*> Components() const
        {
            return AzToolsFramework::GetEntityById(m_entityId)->FindComponents<InspectorTallTestComponent>();
        }

        //! The cards as the user sees them: titles, labels, the numbers shown, heights
        AZStd::vector<CardRecord> Record() const
        {
            using EntityPropertyEditorLazyCardsTestsInternal::ShownTexts;
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            AZStd::vector<CardRecord> records;
            for (const AzToolsFramework::ComponentEditor* card : Cards())
            {
                records.push_back({ card->GetHeader()->titleLabel()->text(), ShownTexts<AzQtComponents::ElidingLabel>(card), card->height(),
                    ShownTexts<QAbstractSpinBox>(card) });
            }
            return records;
        }

        static void ExpectSameCards(const AZStd::vector<CardRecord>& expected, const AZStd::vector<CardRecord>& actual)
        {
            ASSERT_EQ(actual.size(), expected.size());
            for (size_t index = 0; index < expected.size(); ++index)
            {
                EXPECT_EQ(actual[index].m_title, expected[index].m_title) << "card " << index;
                EXPECT_EQ(actual[index].m_labels, expected[index].m_labels) << "card " << index;
                EXPECT_EQ(actual[index].m_numbers, expected[index].m_numbers) << "card " << index;
                EXPECT_EQ(actual[index].m_height, expected[index].m_height) << "card " << index;
            }
        }

        //! Tab from the first stop of the first card until the walk leaves the cards: each stop as (card, stop in the card), the
        //! card's stops counted when the focus is on it (lazy cards build their rows as the focus reaches them)
        AZStd::vector<AZStd::pair<int, int>> WalkTheTabChain()
        {
            using EntityPropertyEditorLazyCardsTestsInternal::IsTabStop;
            const auto stopOf = [this](const QWidget* focused) -> AZStd::pair<int, int>
            {
                const AZStd::vector<AzToolsFramework::ComponentEditor*> cards = Cards();
                for (int cardIndex = 0; cardIndex < static_cast<int>(cards.size()); ++cardIndex)
                {
                    int stopIndex = 0;
                    for (const QWidget* widget : cards[cardIndex]->findChildren<QWidget*>())
                    {
                        if (IsTabStop(widget) && widget->isVisibleTo(m_inspector.get()) && widget->isEnabled())
                        {
                            if (widget == focused)
                            {
                                return { cardIndex, stopIndex };
                            }
                            ++stopIndex;
                        }
                    }
                }
                return { -1, -1 };
            };

            // the walk starts on the first input of the first card's rows (X of Translate), as a user's Tab through the values does
            AZStd::vector<AZStd::pair<int, int>> walk;
            const AZStd::vector<AzToolsFramework::ComponentEditor*> cards = Cards();
            const auto* rows = cards.front()->findChild<AzToolsFramework::DocumentPropertyEditor*>();
            if (!rows)
            {
                return walk;
            }
            QWidget* first = nullptr;
            for (QWidget* widget : rows->findChildren<QWidget*>())
            {
                if (IsTabStop(widget) && widget->isVisibleTo(m_inspector.get()) && widget->isEnabled())
                {
                    first = widget;
                    break;
                }
            }
            if (!first)
            {
                return walk;
            }
            first->setFocus(Qt::TabFocusReason);
            for (int step = 0; step < 200; ++step)
            {
                QWidget* focused = m_inspector->focusWidget();
                const AZStd::pair<int, int> stop = stopOf(focused);
                walk.push_back(stop);
                if (stop.first < 0)
                {
                    break;
                }
                QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
                QApplication::sendEvent(focused, &tab);
            }
            return walk;
        }

        AZ::EntityId m_entityId;
    };

    TEST_F(EntityPropertyEditorLazyCardsTests, TheCardsPastTheViewHoldTheirRowsAndCompleteToTheInspectorOfAFullBuild)
    {
        Show(false);
        const AZStd::vector<CardRecord> fullBuild = Record();
        ASSERT_EQ(fullBuild.size(), static_cast<size_t>(TestComponents + 1));     // the transform and the test components
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);

        Show(true);
        const int held = m_inspector->GetLazyCardsPending();
        EXPECT_GT(held, 0);
        EXPECT_LT(held, TestComponents + 1);                                    // the first cards built their rows
        EXPECT_GT(Record().front().m_labels.size(), 1);                         // the first card shows its rows already

        m_inspector->CompleteLazyCards();
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);
        ExpectSameCards(fullBuild, Record());
    }

    TEST_F(EntityPropertyEditorLazyCardsTests, AHeldMultiEditCardGeneratesItsInstancesContentsOnlyWhenItIsBuilt)
    {
        // a second entity of the same components: the inspector edits both at once (one card per component, one adapter per entity)
        const AZ::EntityId secondEntityId = CreateEntityOf<InspectorTallTestComponent>("LazyCardsSecondEntity", TestComponents);

        // the inspector shows one entity, then both; the contents generated by the second rebuild are counted (verify mode counts
        // every generation)
        AZ::u64 generations = 0;
        AZ::u64 mismatches = 0;
        const auto rebuildBoth = [&](bool lazy)
        {
            Show(lazy);
            AZ::DocumentPropertyEditor::ReflectionAdapter::TakeVerifyCounts(generations, mismatches);
            Select({ m_entityId, secondEntityId });
            AZ::DocumentPropertyEditor::ReflectionAdapter::TakeVerifyCounts(generations, mismatches);
        };
        SetCvar("ed_dpeVerifyContentCaches", true);

        rebuildBoth(false);
        const AZStd::vector<CardRecord> fullBuild = Record();
        ASSERT_EQ(fullBuild.size(), static_cast<size_t>(TestComponents + 1));
        EXPECT_EQ(generations, 2u * fullBuild.size());     // every card, both entities

        // with lazy cards, only the cards the rebuild builds generate their instances' contents; a held card does when it is built
        rebuildBoth(true);
        const AZ::u64 held = m_inspector->GetLazyCardsPending();
        ASSERT_GT(held, 0u);
        EXPECT_EQ(generations, 2u * (fullBuild.size() - held));

        m_inspector->CompleteLazyCards();
        AZ::DocumentPropertyEditor::ReflectionAdapter::TakeVerifyCounts(generations, mismatches);
        EXPECT_EQ(generations, 2u * held);
        EXPECT_EQ(mismatches, 0u);
        ExpectSameCards(fullBuild, Record());
    }

    TEST_F(EntityPropertyEditorLazyCardsTests, TheIdleStepsBuildEveryHeldCard)
    {
        Show(false);
        const AZStd::vector<CardRecord> fullBuild = Record();

        Show(true);
        EXPECT_GT(m_inspector->GetLazyCardsPending(), 0);
        RunTheIdleSteps();
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);
        ExpectSameCards(fullBuild, Record());
    }

    TEST_F(EntityPropertyEditorLazyCardsTests, ARebuildQueuedBeforeAComponentGoesCancelsTheStepsLeft)
    {
        Show(true);
        ASSERT_GT(m_inspector->GetLazyCardsPending(), 0);

        // as DeleteComponents does: the full refresh is asked for first, then the component goes. No held card may build in
        // between (its adapter would read the deleted component).
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
        AZ::Entity* entity = AzToolsFramework::GetEntityById(m_entityId);
        entity->Deactivate();
        InspectorTallTestComponent* last = Components().back();
        entity->RemoveComponent(last);
        delete last;
        entity->Activate();

        RunTheIdleSteps();      // the queued rebuild runs, then the idle steps of its own cards
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);
        EXPECT_EQ(Cards().size(), static_cast<size_t>(TestComponents));
    }

    TEST_F(EntityPropertyEditorLazyCardsTests, ACollapsedCardBuildsItsRowsWhenItIsExpanded)
    {
        // a window tall enough for every card: only the collapsed one holds its rows
        AzToolsFramework::EditorEntityInfoRequestBus::Event(
            m_entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::SetComponentExpanded, Components().front()->GetId(), false);
        Show(true, 4000);
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 1);

        AzToolsFramework::ComponentEditor* collapsed = nullptr;
        for (AzToolsFramework::ComponentEditor* card : Cards())
        {
            if (card->IsContentBuildHeld())
            {
                collapsed = card;
            }
        }
        ASSERT_NE(collapsed, nullptr);
        EXPECT_FALSE(collapsed->IsExpanded());

        collapsed->SetExpanded(true);
        EXPECT_FALSE(collapsed->IsContentBuildHeld());
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);
    }

    TEST_F(EntityPropertyEditorLazyCardsTests, TheTabWalkAcrossTheCardsIsTheFullBuildsWithTheCardsBuiltOrStillHeld)
    {
        // the full build's walk goes through the values of every card (the transform's eleven stops, then eight per test card)
        Show(false);
        const AZStd::vector<AZStd::pair<int, int>> fullBuild = WalkTheTabChain();
        ASSERT_GE(fullBuild.size(), static_cast<size_t>(11 + 8 * TestComponents));
        EXPECT_EQ(fullBuild[fullBuild.size() - 2].first, TestComponents);      // the last stop before the walk leaves is in the last card

        // every card built at idle first
        Show(true);
        RunTheIdleSteps();
        EXPECT_EQ(WalkTheTabChain(), fullBuild);

        // the walk reaching cards that still hold their rows: the list scrolls to the focus, and the scroll builds the cards down to
        // one screen below it before the focus gets there
        Show(true);
        const int heldBeforeTheWalk = m_inspector->GetLazyCardsPending();
        ASSERT_GT(heldBeforeTheWalk, 0);
        EXPECT_EQ(WalkTheTabChain(), fullBuild);
        EXPECT_LT(m_inspector->GetLazyCardsPending(), heldBeforeTheWalk);     // the walk crossed held cards
    }

    TEST_F(EntityPropertyEditorLazyCardsTests, TheRestoredScrollPositionShowsTheSameCardAsAFullBuild)
    {
        // a rebuild restores the scroll position the inspector had: the first slice builds the cards down to it
        const auto scrolledRebuild = [this](bool lazy)
        {
            Show(lazy);
            m_inspector->CompleteLazyCards();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QScrollBar* scroll = m_inspector->findChild<QScrollArea*>("m_componentList")->verticalScrollBar();
            scroll->setValue(scroll->maximum() * 2 / 3);
            const int value = scroll->value();

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
            for (int turn = 0; turn < 10; ++turn)
            {
                QApplication::processEvents();     // the queued rebuild, then the restore it queues
            }

            // the card at the top of the view
            QString topTitle;
            for (const AzToolsFramework::ComponentEditor* card : Cards())
            {
                if (card->geometry().bottom() >= scroll->value())
                {
                    topTitle = card->GetHeader()->titleLabel()->text();
                    break;
                }
            }
            return AZStd::make_tuple(value, scroll->value(), topTitle);
        };

        const auto [fullValueSet, fullValue, fullTop] = scrolledRebuild(false);
        const auto [lazyValueSet, lazyValue, lazyTop] = scrolledRebuild(true);
        EXPECT_EQ(lazyValueSet, fullValueSet);
        EXPECT_EQ(lazyValue, fullValue);
        EXPECT_EQ(lazyTop, fullTop);
        EXPECT_GT(fullValue, 0);
    }
} // namespace UnitTest
