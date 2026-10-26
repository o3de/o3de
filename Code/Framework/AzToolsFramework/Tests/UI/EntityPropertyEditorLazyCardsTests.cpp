/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/tuple.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <Prefab/PrefabTestFixture.h>

#include <QAbstractSpinBox>
#include <QApplication>
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
    // The inspector's lazy cards (ed_inspectorLazyCards, inspector build plan I.4b): the cards past the view and collapsed cards
    // hold their rows; the rows are built top to bottom later. Once built, the inspector is the one a full build gives.
    // ----------------------------------------------------------------------------------

    namespace EntityPropertyEditorLazyCardsTestsInternal
    {
        //! A component with a tall card: eight numbers
        class LazyCardsTestComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                LazyCardsTestComponent, "{7B1E7A2C-0C5D-4D5E-9E3B-5B8E6E2F4A11}", AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<LazyCardsTestComponent, AzToolsFramework::Components::EditorComponentBase>()
                        ->Version(1)
                        ->Field("value0", &LazyCardsTestComponent::m_value0)
                        ->Field("value1", &LazyCardsTestComponent::m_value1)
                        ->Field("value2", &LazyCardsTestComponent::m_value2)
                        ->Field("value3", &LazyCardsTestComponent::m_value3)
                        ->Field("value4", &LazyCardsTestComponent::m_value4)
                        ->Field("value5", &LazyCardsTestComponent::m_value5)
                        ->Field("value6", &LazyCardsTestComponent::m_value6)
                        ->Field("value7", &LazyCardsTestComponent::m_value7);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<LazyCardsTestComponent>("Lazy Cards Test", "A tall card")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value0, "Value 0", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value1, "Value 1", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value2, "Value 2", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value3, "Value 3", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value4, "Value 4", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value5, "Value 5", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value6, "Value 6", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LazyCardsTestComponent::m_value7, "Value 7", "");
                    }
                }
            }

            float m_value0 = 0.0f;
            float m_value1 = 1.0f;
            float m_value2 = 2.0f;
            float m_value3 = 3.0f;
            float m_value4 = 4.0f;
            float m_value5 = 5.0f;
            float m_value6 = 6.0f;
            float m_value7 = 7.0f;
        };

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
    } // namespace EntityPropertyEditorLazyCardsTestsInternal

    class EntityPropertyEditorLazyCardsTests : public PrefabTestFixture
    {
    protected:
        using LazyCardsTestComponent = EntityPropertyEditorLazyCardsTestsInternal::LazyCardsTestComponent;
        using CardRecord = EntityPropertyEditorLazyCardsTestsInternal::CardRecord;

        static constexpr int TestComponents = 6;

        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();
            GetApplication()->RegisterComponentDescriptor(LazyCardsTestComponent::CreateDescriptor());

            // the transform and six tall cards: the inspector of a short window shows the first two
            m_entityId = CreateEditorEntityUnderRoot("LazyCardsEntity");
            AZ::Entity* entity = AzToolsFramework::GetEntityById(m_entityId);
            entity->Deactivate();
            for (int index = 0; index < TestComponents; ++index)
            {
                LazyCardsTestComponent* component = entity->CreateComponent<LazyCardsTestComponent>();
                component->SetSerializedIdentifier(AZStd::string::format("LazyCardsTestComponent_%d", index));
                m_components.push_back(component);
            }
            entity->Activate();
        }

        void TearDownEditorFixtureImpl() override
        {
            m_inspector.reset();
            SetLazyCards(true);     // the default
            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        static void SetLazyCards(bool lazy)
        {
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(lazy ? "ed_inspectorLazyCards true" : "ed_inspectorLazyCards false");
        }

        //! A new inspector of the given height, shown, with lazy cards on or off, showing the entity (its rebuild has just run)
        void Show(bool lazy, int windowHeight = 320)
        {
            m_inspector.reset();
            SetLazyCards(lazy);
            m_inspector = AZStd::make_unique<AzToolsFramework::EntityPropertyEditor>(nullptr, Qt::WindowFlags(), false);
            m_inspector->setAttribute(Qt::WA_DontShowOnScreen);
            m_inspector->resize(420, windowHeight);
            m_inspector->show();
            QApplication::processEvents();
            Rebuild();
        }

        //! The rebuild a new selection runs
        void Rebuild()
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{ m_entityId });
            QMetaObject::invokeMethod(m_inspector.get(), "UpdateContents", Qt::DirectConnection);
        }

        //! Lets the event loop run until no card holds its rows (the idle steps), within a bound
        void RunTheIdleSteps()
        {
            for (int turn = 0; turn < 1000 && m_inspector->GetLazyCardsPending() > 0; ++turn)
            {
                QApplication::processEvents();
            }
        }

        //! The inspector's shown cards, top to bottom
        AZStd::vector<AzToolsFramework::ComponentEditor*> Cards() const
        {
            AZStd::vector<AzToolsFramework::ComponentEditor*> cards;
            for (AzToolsFramework::ComponentEditor* card : m_inspector->findChildren<AzToolsFramework::ComponentEditor*>())
            {
                if (card->isVisible())
                {
                    cards.push_back(card);
                }
            }
            AZStd::sort(cards.begin(), cards.end(),
                [](const QWidget* a, const QWidget* b)
                {
                    return a->mapTo(a->window(), QPoint(0, 0)).y() < b->mapTo(b->window(), QPoint(0, 0)).y();
                });
            return cards;
        }

        //! The cards as the user sees them: titles, labels, the numbers shown, heights
        AZStd::vector<CardRecord> Record() const
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            AZStd::vector<CardRecord> records;
            for (const AzToolsFramework::ComponentEditor* card : Cards())
            {
                CardRecord record;
                record.m_title = card->GetHeader()->titleLabel()->text();
                QList<AzQtComponents::ElidingLabel*> labels = card->findChildren<AzQtComponents::ElidingLabel*>();
                AZStd::sort(labels.begin(), labels.end(),
                    [](const QWidget* a, const QWidget* b)
                    {
                        return a->mapTo(a->window(), QPoint(0, 0)).y() < b->mapTo(b->window(), QPoint(0, 0)).y();
                    });
                for (const AzQtComponents::ElidingLabel* label : labels)
                {
                    if (label->isVisible())
                    {
                        record.m_labels.push_back(label->text());
                    }
                }
                record.m_height = card->height();
                QList<QAbstractSpinBox*> spinBoxes = card->findChildren<QAbstractSpinBox*>();
                AZStd::sort(spinBoxes.begin(), spinBoxes.end(),
                    [](const QWidget* a, const QWidget* b)
                    {
                        return a->mapTo(a->window(), QPoint(0, 0)).y() < b->mapTo(b->window(), QPoint(0, 0)).y();
                    });
                for (const QAbstractSpinBox* spinBox : spinBoxes)
                {
                    if (spinBox->isVisible())
                    {
                        record.m_numbers.push_back(spinBox->text());
                    }
                }
                records.push_back(AZStd::move(record));
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
        AZStd::vector<LazyCardsTestComponent*> m_components;
        AZStd::unique_ptr<AzToolsFramework::EntityPropertyEditor> m_inspector;
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
        const AZ::EntityId secondEntityId = CreateEditorEntityUnderRoot("LazyCardsSecondEntity");
        AZ::Entity* secondEntity = AzToolsFramework::GetEntityById(secondEntityId);
        secondEntity->Deactivate();
        for (int index = 0; index < TestComponents; ++index)
        {
            secondEntity->CreateComponent<LazyCardsTestComponent>()->SetSerializedIdentifier(
                AZStd::string::format("LazyCardsTestComponent_%d", index));
        }
        secondEntity->Activate();

        // the inspector shows one entity, then both; the contents generated by the second rebuild are counted (verify mode counts
        // every generation)
        AZ::u64 generations = 0;
        AZ::u64 mismatches = 0;
        const auto rebuildBoth = [&](bool lazy)
        {
            Show(lazy);
            AZ::DocumentPropertyEditor::ReflectionAdapter::TakeVerifyCounts(generations, mismatches);
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                AzToolsFramework::EntityIdList{ m_entityId, secondEntityId });
            QMetaObject::invokeMethod(m_inspector.get(), "UpdateContents", Qt::DirectConnection);
            AZ::DocumentPropertyEditor::ReflectionAdapter::TakeVerifyCounts(generations, mismatches);
        };
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("ed_dpeVerifyContentCaches true");

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

        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("ed_dpeVerifyContentCaches false");
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
        LazyCardsTestComponent* last = m_components.back();
        entity->RemoveComponent(last);
        delete last;
        m_components.pop_back();
        entity->Activate();

        RunTheIdleSteps();      // the queued rebuild runs, then the idle steps of its own cards
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);
        EXPECT_EQ(Cards().size(), static_cast<size_t>(TestComponents));
    }

    TEST_F(EntityPropertyEditorLazyCardsTests, ACollapsedCardBuildsItsRowsWhenItIsExpanded)
    {
        // a window tall enough for every card: only the collapsed one holds its rows
        AzToolsFramework::EditorEntityInfoRequestBus::Event(
            m_entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::SetComponentExpanded, m_components[0]->GetId(), false);
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
