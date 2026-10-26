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
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <Prefab/PrefabTestFixture.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/sort.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollArea>
#include <QWidget>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The inspector's interruptible load (ed_inspectorInterruptibleLoad, inspector build plan I.7): a new selection shows (in the
    // outliner, or any widget that repaints for it) before the inspector rebuilds for it; the rebuild a selection queues holds every
    // card, keeps the inspector's last picture until the idle steps have built the cards in view, and a newer selection drops the
    // steps left.
    // ----------------------------------------------------------------------------------

    namespace EntityPropertyEditorInterruptibleLoadTestsInternal
    {
        //! A short card: one number
        class InterruptibleShortTestComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                InterruptibleShortTestComponent, "{92DC5E55-5441-43A7-9942-418364F38310}", AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<InterruptibleShortTestComponent, AzToolsFramework::Components::EditorComponentBase>()
                        ->Version(1)
                        ->Field("value", &InterruptibleShortTestComponent::m_value);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<InterruptibleShortTestComponent>("Interruptible Short Test", "A short card")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &InterruptibleShortTestComponent::m_value, "Value", "");
                    }
                }
            }

            float m_value = 0.0f;
        };

        //! A tall card: eight numbers
        class InterruptibleTallTestComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                InterruptibleTallTestComponent, "{0E2B6425-2C80-4979-A066-2F1FD0B218F9}", AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<InterruptibleTallTestComponent, AzToolsFramework::Components::EditorComponentBase>()
                        ->Version(1)
                        ->Field("values", &InterruptibleTallTestComponent::m_values);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<InterruptibleTallTestComponent>("Interruptible Tall Test", "A tall card")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &InterruptibleTallTestComponent::m_values, "Values", "");
                    }
                }
            }

            AZStd::vector<float> m_values = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
        };

        //! A slow card: one number whose visibility takes longer to read than a lazy cards step's budget (10 ms)
        class InterruptibleSlowTestComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                InterruptibleSlowTestComponent, "{3828131F-6B6A-4059-A36F-83C72CCC4B98}", AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<InterruptibleSlowTestComponent, AzToolsFramework::Components::EditorComponentBase>()
                        ->Version(1)
                        ->Field("value", &InterruptibleSlowTestComponent::m_value);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<InterruptibleSlowTestComponent>("Interruptible Slow Test", "A slow card")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &InterruptibleSlowTestComponent::m_value, "Value", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &InterruptibleSlowTestComponent::SlowVisibility);
                    }
                }
            }

            AZ::Crc32 SlowVisibility() const
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(12));
                return AZ::Edit::PropertyVisibility::Show;
            }

            float m_value = 0.0f;
        };

        //! A window that repaints for a new selection, as the outliner does, and records the inspector's card titles when its paint
        //! runs (the update request is delivered; the window is never on screen)
        class SelectionPaintProbe : public QWidget
        {
        public:
            explicit SelectionPaintProbe(const AZStd::function<QStringList()>& inspectorTitles)
                : m_inspectorTitles(inspectorTitles)
            {
                setAttribute(Qt::WA_DontShowOnScreen);
                resize(64, 64);
            }

            AZStd::vector<QStringList> m_titlesAtPaints;

        protected:
            bool event(QEvent* event) override
            {
                if (event->type() == QEvent::UpdateRequest)
                {
                    m_titlesAtPaints.push_back(m_inspectorTitles());
                }
                return QWidget::event(event);
            }

        private:
            AZStd::function<QStringList()> m_inspectorTitles;
        };
    } // namespace EntityPropertyEditorInterruptibleLoadTestsInternal

    class EntityPropertyEditorInterruptibleLoadTests : public PrefabTestFixture
    {
    protected:
        using InterruptibleShortTestComponent = EntityPropertyEditorInterruptibleLoadTestsInternal::InterruptibleShortTestComponent;
        using InterruptibleTallTestComponent = EntityPropertyEditorInterruptibleLoadTestsInternal::InterruptibleTallTestComponent;
        using InterruptibleSlowTestComponent = EntityPropertyEditorInterruptibleLoadTestsInternal::InterruptibleSlowTestComponent;
        using SelectionPaintProbe = EntityPropertyEditorInterruptibleLoadTestsInternal::SelectionPaintProbe;

        static constexpr int TallComponents = 6;
        static constexpr int SlowComponents = 4;

        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();
            GetApplication()->RegisterComponentDescriptor(InterruptibleShortTestComponent::CreateDescriptor());
            GetApplication()->RegisterComponentDescriptor(InterruptibleTallTestComponent::CreateDescriptor());
            GetApplication()->RegisterComponentDescriptor(InterruptibleSlowTestComponent::CreateDescriptor());

            // one entity of the transform and a short card, one of the transform and six tall cards, one of the transform and four
            // slow cards
            m_shortEntityId = CreateEditorEntityUnderRoot("InterruptibleShortEntity");
            AZ::Entity* shortEntity = AzToolsFramework::GetEntityById(m_shortEntityId);
            shortEntity->Deactivate();
            shortEntity->CreateComponent<InterruptibleShortTestComponent>()->SetSerializedIdentifier("InterruptibleShortTestComponent_0");
            shortEntity->Activate();

            m_tallEntityId = CreateEditorEntityUnderRoot("InterruptibleTallEntity");
            AZ::Entity* tallEntity = AzToolsFramework::GetEntityById(m_tallEntityId);
            tallEntity->Deactivate();
            for (int index = 0; index < TallComponents; ++index)
            {
                tallEntity->CreateComponent<InterruptibleTallTestComponent>()->SetSerializedIdentifier(
                    AZStd::string::format("InterruptibleTallTestComponent_%d", index));
            }
            tallEntity->Activate();

            m_slowEntityId = CreateEditorEntityUnderRoot("InterruptibleSlowEntity");
            AZ::Entity* slowEntity = AzToolsFramework::GetEntityById(m_slowEntityId);
            slowEntity->Deactivate();
            for (int index = 0; index < SlowComponents; ++index)
            {
                slowEntity->CreateComponent<InterruptibleSlowTestComponent>()->SetSerializedIdentifier(
                    AZStd::string::format("InterruptibleSlowTestComponent_%d", index));
            }
            slowEntity->Activate();
        }

        void TearDownEditorFixtureImpl() override
        {
            m_inspector.reset();
            SetInterruptibleLoad(true);     // the defaults
            SetInlineMs(40);
            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        //! ed_inspectorInterruptibleInlineMs: how long the rebuild builds the cards in view before it lets the event loop run
        static void SetInlineMs(int ms)
        {
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(AZStd::string::format("ed_inspectorInterruptibleInlineMs %d", ms).c_str());
        }

        static void SetInterruptibleLoad(bool interruptible)
        {
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(
                interruptible ? "ed_inspectorInterruptibleLoad true" : "ed_inspectorInterruptibleLoad false");
        }

        //! A new inspector, shown, showing the entity once the event loop has run its rebuild
        void Show(AZ::EntityId entityId)
        {
            m_inspector.reset();
            m_inspector = AZStd::make_unique<AzToolsFramework::EntityPropertyEditor>(nullptr, Qt::WindowFlags(), false);
            m_inspector->setAttribute(Qt::WA_DontShowOnScreen);
            m_inspector->resize(420, 320);
            m_inspector->show();
            QApplication::processEvents();
            SelectQueued(entityId);
            RunTheEventLoop();
        }

        //! A selection change as the outliner or the viewport makes it: the inspector queues its rebuild
        static void SelectQueued(AZ::EntityId entityId)
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{ entityId });
        }

        static void RunTheEventLoop()
        {
            QElapsedTimer timer;
            timer.start();
            while (timer.elapsed() < 100)
            {
                QApplication::processEvents();
            }
        }

        //! Runs the rebuild the selection queued, and nothing after it: the inspector as the event loop leaves it after that turn
        void RunTheQueuedRebuildOnly()
        {
            QCoreApplication::sendPostedEvents(m_inspector.get(), QEvent::MetaCall);
        }

        //! Runs the idle step the inspector queued last, and nothing after it
        void RunTheQueuedStepOnly()
        {
            QCoreApplication::sendPostedEvents(m_inspector.get(), QEvent::MetaCall);
        }

        //! The event loop's turns until the inspector shows its cards again (the freeze ends), within a bound
        void RunUntilTheFreezeEnds()
        {
            for (int turn = 0; turn < 1000 && !m_inspector->updatesEnabled(); ++turn)
            {
                QApplication::processEvents();
            }
        }

        //! The event loop's turns until no card holds its rows, within a bound
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
                    return a->geometry().y() < b->geometry().y();
                });
            return cards;
        }

        //! The cards that start in the view, laid out as the event loop will lay them out
        AZStd::vector<AzToolsFramework::ComponentEditor*> CardsInView() const
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            const QWidget* viewport = m_inspector->findChild<QScrollArea*>("m_componentList")->viewport();
            AZStd::vector<AzToolsFramework::ComponentEditor*> inView;
            for (AzToolsFramework::ComponentEditor* card : Cards())
            {
                if (card->geometry().top() < viewport->height())
                {
                    inView.push_back(card);
                }
            }
            return inView;
        }

        //! Titles and heights of the shown cards, top to bottom
        AZStd::vector<AZStd::pair<QString, int>> TitlesAndHeights() const
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            AZStd::vector<AZStd::pair<QString, int>> record;
            for (const AzToolsFramework::ComponentEditor* card : Cards())
            {
                record.emplace_back(card->GetHeader()->titleLabel()->text(), card->height());
            }
            return record;
        }

        //! The titles of the inspector's shown cards, in no particular order
        QStringList Titles() const
        {
            QStringList titles;
            for (const AzToolsFramework::ComponentEditor* card : m_inspector->findChildren<AzToolsFramework::ComponentEditor*>())
            {
                if (card->isVisible())
                {
                    titles.push_back(card->GetHeader()->titleLabel()->text());
                }
            }
            titles.sort();
            return titles;
        }

        //! The titles the probe saw at its first paint after a selection change: the paint the selection causes
        QStringList TitlesAtTheSelectionsPaint()
        {
            SelectionPaintProbe probe([this]() { return Titles(); });
            probe.show();
            RunTheEventLoop();
            probe.m_titlesAtPaints.clear();

            // the outliner's new selection: its update() posts an update request to its window at low priority, as Qt's repaint
            // manager does (a window never on screen posts none, so the test posts it)
            SelectQueued(m_tallEntityId);
            QCoreApplication::postEvent(&probe, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
            RunTheEventLoop();
            EXPECT_FALSE(probe.m_titlesAtPaints.empty());
            return probe.m_titlesAtPaints.empty() ? QStringList() : probe.m_titlesAtPaints.front();
        }

        AZ::EntityId m_shortEntityId;
        AZ::EntityId m_tallEntityId;
        AZ::EntityId m_slowEntityId;
        AZStd::unique_ptr<AzToolsFramework::EntityPropertyEditor> m_inspector;
    };

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, ANewSelectionPaintsBeforeTheInspectorRebuildsForIt)
    {
        Show(m_shortEntityId);
        const QStringList shortTitles = Titles();
        ASSERT_EQ(shortTitles.size(), 2);     // the transform and the short card

        // the paint the selection causes comes before the rebuild: it still sees the previous entity's cards
        EXPECT_EQ(TitlesAtTheSelectionsPaint(), shortTitles);
        EXPECT_EQ(Titles().size(), 1 + TallComponents);         // and the inspector then shows the new entity
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, WithoutInterruptibleLoadTheInspectorRebuildsBeforeTheSelectionPaints)
    {
        // the behaviour the first paint replaces (and the proof that the test above can tell the two apart)
        SetInterruptibleLoad(false);
        Show(m_shortEntityId);
        const QStringList shortTitles = Titles();

        const QStringList atPaint = TitlesAtTheSelectionsPaint();
        EXPECT_NE(atPaint, shortTitles);
        EXPECT_EQ(atPaint.size(), 1 + TallComponents);
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, AQueuedRebuildKeepsTheLastPictureUntilTheIdleStepsBuildTheView)
    {
        SetInlineMs(0);     // every card in idle steps: the freeze
        // the inspector a synchronous rebuild gives, every card built
        Show(m_tallEntityId);
        QMetaObject::invokeMethod(m_inspector.get(), "UpdateContents", Qt::DirectConnection);
        m_inspector->CompleteLazyCards();
        const AZStd::vector<AZStd::pair<QString, int>> fullBuild = TitlesAndHeights();
        ASSERT_EQ(fullBuild.size(), static_cast<size_t>(1 + TallComponents));

        // a selection's rebuild returns with every card held (their headers shown) and the inspector frozen
        SelectQueued(m_shortEntityId);
        RunTheEventLoop();
        SelectQueued(m_tallEntityId);
        RunTheQueuedRebuildOnly();
        EXPECT_FALSE(m_inspector->updatesEnabled());
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 1 + TallComponents);
        EXPECT_EQ(Titles().size(), 1 + TallComponents);

        // the idle steps build the cards in view, then the inspector shows them; the cards below it are built after
        RunUntilTheFreezeEnds();
        EXPECT_TRUE(m_inspector->updatesEnabled());
        for (const AzToolsFramework::ComponentEditor* card : CardsInView())
        {
            EXPECT_FALSE(card->IsContentBuildHeld()) << card->GetHeader()->titleLabel()->text().toUtf8().constData();
        }
        EXPECT_GT(m_inspector->GetLazyCardsPending(), 0);

        RunTheIdleSteps();
        EXPECT_EQ(TitlesAndHeights(), fullBuild);
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, ANewSelectionDuringTheStepsDropsTheRestAndBuildsTheNewOne)
    {
        SetInlineMs(0);     // every card in idle steps: the freeze
        Show(m_shortEntityId);
        const QStringList shortTitles = Titles();

        SelectQueued(m_tallEntityId);
        RunTheQueuedRebuildOnly();
        ASSERT_FALSE(m_inspector->updatesEnabled());
        RunTheQueuedRebuildOnly();      // the tall entity's first idle step

        // the user moves on before the tall entity is built
        SelectQueued(m_shortEntityId);
        RunTheEventLoop();
        EXPECT_TRUE(m_inspector->updatesEnabled());
        EXPECT_EQ(Titles(), shortTitles);
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, TheFrozenInspectorIgnoresTheMouse)
    {
        SetInlineMs(0);     // every card in idle steps: the freeze
        //! Counts the presses a widget receives
        class PressCounter : public QObject
        {
        public:
            bool eventFilter(QObject* watched, QEvent* event) override
            {
                m_presses += event->type() == QEvent::MouseButtonPress ? 1 : 0;
                return QObject::eventFilter(watched, event);
            }
            int m_presses = 0;
        };

        Show(m_shortEntityId);
        SelectQueued(m_tallEntityId);
        RunTheQueuedRebuildOnly();
        ASSERT_FALSE(m_inspector->updatesEnabled());

        QWidget* header = Cards().front()->GetHeader();
        PressCounter counter;
        header->installEventFilter(&counter);
        const auto press = [header]()
        {
            QMouseEvent event(QEvent::MouseButtonPress, QPointF(5, 5), header->mapToGlobal(QPointF(5, 5)), Qt::LeftButton, Qt::LeftButton,
                Qt::NoModifier);
            QApplication::sendEvent(header, &event);
        };
        press();
        EXPECT_EQ(counter.m_presses, 0);    // the card under the mouse is not the one shown

        RunUntilTheFreezeEnds();
        press();
        EXPECT_EQ(counter.m_presses, 1);
        header->removeEventFilter(&counter);
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, CompletingTheLazyCardsEndsTheFreeze)
    {
        SetInlineMs(0);     // every card in idle steps: the freeze
        Show(m_shortEntityId);
        SelectQueued(m_tallEntityId);
        RunTheQueuedRebuildOnly();
        ASSERT_FALSE(m_inspector->updatesEnabled());

        m_inspector->CompleteLazyCards();
        EXPECT_TRUE(m_inspector->updatesEnabled());
        EXPECT_EQ(m_inspector->GetLazyCardsPending(), 0);
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, TheRebuildBuildsTheViewItselfWithinItsInlineBudget)
    {
        // a budget the view of this short window fits in: the rebuild shows it without a freeze; the cards below it wait
        SetInlineMs(1000);
        Show(m_shortEntityId);
        SelectQueued(m_tallEntityId);
        RunTheQueuedRebuildOnly();
        EXPECT_TRUE(m_inspector->updatesEnabled());
        for (const AzToolsFramework::ComponentEditor* card : CardsInView())
        {
            EXPECT_FALSE(card->IsContentBuildHeld());
        }
        EXPECT_GT(m_inspector->GetLazyCardsPending(), 0);
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, AStepOfTheFrozenInspectorBuildsTheViewForTheInlineBudget)
    {
        // a rebuild that builds no card itself: the inspector frozen, every card held; the window tall enough for every slow card
        SetInlineMs(0);
        Show(m_shortEntityId);
        m_inspector->resize(420, 900);
        RunTheEventLoop();
        SelectQueued(m_slowEntityId);
        RunTheQueuedRebuildOnly();
        ASSERT_FALSE(m_inspector->updatesEnabled());
        ASSERT_EQ(m_inspector->GetLazyCardsPending(), 1 + SlowComponents);

        // the next step builds the cards in view for the inline budget (a lazy cards step's 10 ms would build one slow card): all of
        // them, and the inspector shows them
        SetInlineMs(1000);
        RunTheQueuedStepOnly();
        EXPECT_TRUE(m_inspector->updatesEnabled());
        const AZStd::vector<AzToolsFramework::ComponentEditor*> inView = CardsInView();
        EXPECT_EQ(inView.size(), static_cast<size_t>(1 + SlowComponents));
        for (const AzToolsFramework::ComponentEditor* card : inView)
        {
            EXPECT_FALSE(card->IsContentBuildHeld()) << card->GetHeader()->titleLabel()->text().toUtf8().constData();
        }
    }

    TEST_F(EntityPropertyEditorInterruptibleLoadTests, ADirectRebuildStaysSynchronous)
    {
        // UpdateContents called directly (the constructor, pinned inspectors, tests) builds the cards in view at once
        Show(m_shortEntityId);
        SelectQueued(m_tallEntityId);
        QMetaObject::invokeMethod(m_inspector.get(), "UpdateContents", Qt::DirectConnection);
        EXPECT_TRUE(m_inspector->updatesEnabled());
        EXPECT_LT(m_inspector->GetLazyCardsPending(), 1 + TallComponents);
        for (const AzToolsFramework::ComponentEditor* card : CardsInView())
        {
            EXPECT_FALSE(card->IsContentBuildHeld());
        }
    }
} // namespace UnitTest
