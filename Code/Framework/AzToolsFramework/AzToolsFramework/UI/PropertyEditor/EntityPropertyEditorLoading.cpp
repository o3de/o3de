/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// The Entity Inspector's loading (EntityPropertyEditor): the layout of its component list, the lazy cards, and the interruptible load
// and its view freeze. EntityPropertyEditor.cpp calls in from its rebuild, its cards' signals and its event filter.

#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>

#include <AzCore/Console/IConsole.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ui_EntityPropertyEditor.h>

#include <QCoreApplication>
#include <QScrollBar>
#include <QTimer>

namespace AzToolsFramework
{
    // ----------------------------------------------------------------------------------
    // Console variables: the features' switches and budgets
    // ----------------------------------------------------------------------------------

    AZ_CVAR(
        bool,
        ed_inspectorSettleLayout,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set, the Entity Inspector also lays its component list out before it paints at the end of a rebuild and of each idle lazy "
        "cards step: a reused card then never shows where the previous selection's card was. It always does after an expand, a "
        "collapse, or cards built as the view reaches them.");

    AZ_CVAR(
        bool,
        ed_inspectorLazyCards,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set (the default), the Entity Inspector builds the rows of the cards in view (and one screen below) at once, and those of "
        "the cards past them and of collapsed cards later, top to bottom: at idle, or as soon as the view or the focus reaches them.");

    //! A lazy cards step builds held cards for about this long (at least one card) before it lets the event loop run
    static constexpr AZ::s64 LazyCardsStepBudgetUs = 10000;

    AZ_CVAR(
        bool,
        ed_inspectorInterruptibleLoad,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set (the default), the Entity Inspector lets a new selection show before it rebuilds, and a newer one interrupt the "
        "rebuild: the rebuild a selection queues paints what waits to be painted first (the outliner's new selection), builds every "
        "card's rows in idle steps, and keeps the inspector's last picture until the cards in view are built, then shows them in one "
        "paint; a new selection drops the steps left.");

    AZ_CVAR(
        int,
        ed_inspectorInterruptibleInlineMs,
        40,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "With ed_inspectorInterruptibleLoad, the rebuild a selection queues builds the cards in view for about this long (ms) before "
        "it lets the event loop run (the engine's tick, other windows' paints), and so does each idle step until they show. 0 builds "
        "every card in idle steps.");

    //! ed_inspectorInterruptibleInlineMs in microseconds
    static AZ::s64 InterruptibleInlineBudgetUs()
    {
        return aznumeric_cast<AZ::s64>(static_cast<int>(ed_inspectorInterruptibleInlineMs)) * 1000;
    }

    // ----------------------------------------------------------------------------------
    // Component list layout
    // ----------------------------------------------------------------------------------

    //! Counts the layout requests the application delivers while it filters the application's events
    class LayoutRequestCounter : public QObject
    {
    public:
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            m_delivered += event->type() == QEvent::LayoutRequest ? 1 : 0;
            return QObject::eventFilter(watched, event);
        }

        int m_delivered = 0;
    };

    void EntityPropertyEditor::SettleComponentList(SettleOrigin origin)
    {
        // A layout that changes posts a layout request to the layout above it (a card's rows, the card, the list, the scroll area), and
        // the event loop paints between the rounds: a reused card or an expanded row would show before it is placed. The requests are
        // sent here round after round until none is left. A rebuild and an idle lazy cards step settle only with
        // ed_inspectorSettleLayout; an interaction always does, or the card the user acts on bounces.
        if (m_componentListSettling || (origin == SettleOrigin::Build && !ed_inspectorSettleLayout))
        {
            return;
        }
        m_componentListSettling = true;

        LayoutRequestCounter counter;
        QCoreApplication::instance()->installEventFilter(&counter);
        int rounds = 0;
        while (rounds < SettleRoundsMax)
        {
            counter.m_delivered = 0;
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            if (counter.m_delivered == 0)
            {
                break;
            }
            ++rounds;
        }
        QCoreApplication::instance()->removeEventFilter(&counter);

        m_componentListSettling = false;
    }

    // ----------------------------------------------------------------------------------
    // Lazy cards (ed_inspectorLazyCards)
    // ----------------------------------------------------------------------------------

    int EntityPropertyEditor::GetLazyCardsPending() const
    {
        int held = 0;
        for (AZ::s32 index = 0; index < m_componentEditorsUsed; ++index)
        {
            held += m_componentEditors[index]->IsContentBuildHeld() ? 1 : 0;
        }
        return held;
    }

    void EntityPropertyEditor::ReleaseLazyCardsThrough(int lastIndex, SettleOrigin settleOrigin, AZ::s64 budgetUs)
    {
        if (m_lazyCardsReleasing)
        {
            return;
        }
        m_lazyCardsReleasing = true;

        const AZStd::chrono::steady_clock::time_point releaseStart = AZStd::chrono::steady_clock::now();
        QScrollBar* verticalScroll = m_gui->m_componentList->verticalScrollBar();
        bool released = false;
        for (int index = 0; index <= lastIndex && index < m_componentEditorsUsed; ++index)
        {
            ComponentEditor* componentEditor = m_componentEditors[index];
            if (!componentEditor->IsContentBuildHeld())
            {
                continue;
            }

            const bool aboveView = componentEditor->isVisible() && componentEditor->geometry().bottom() < verticalScroll->value();
            const int heightBefore = componentEditor->height();
            componentEditor->ReleaseContentBuild();
            released = true;

            // A card above the view resizes the list now, so that the view keeps what it shows (the height it gains is only known
            // once the list is laid out, so this settles whatever the origin)
            if (aboveView)
            {
                SettleComponentList(SettleOrigin::Interaction);
                verticalScroll->setValue(verticalScroll->value() + componentEditor->height() - heightBefore);
            }

            // an idle step stops when its budget is spent (after one card at least)
            const AZ::s64 elapsedUs =
                AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - releaseStart).count();
            if (budgetUs > 0 && elapsedUs >= budgetUs)
            {
                break;
            }
        }

        // the built cards at their new heights, and the cards below them moved, before the inspector paints
        if (released)
        {
            SettleComponentList(settleOrigin);
            UpdateLazyCardsPendingProperty();
        }

        m_lazyCardsReleasing = false;
    }

    bool EntityPropertyEditor::CanReleaseLazyCards() const
    {
        // not inside a release, during a rebuild, or with one queued (the held cards' components may be going)
        return !m_lazyCardsReleasing && !m_isBuildingProperties && !m_isAlreadyQueuedRefresh && GetLazyCardsPending() > 0;
    }

    void EntityPropertyEditor::ReleaseLazyCardsInView()
    {
        // While the view is frozen the steps build it: the scroll bar that shows as the list grows (a viewport resize) and the scroll
        // restore must not build the whole view at once
        if (m_viewFrozen || !CanReleaseLazyCards())
        {
            return;
        }

        // the last card that starts in the view or in the screen below it
        const int viewBottom = m_gui->m_componentList->verticalScrollBar()->value() + 2 * m_gui->m_componentList->viewport()->height();
        int lastInView = -1;
        for (AZ::s32 index = 0; index < m_componentEditorsUsed; ++index)
        {
            const ComponentEditor* componentEditor = m_componentEditors[index];
            if (componentEditor->isVisible() && componentEditor->geometry().top() <= viewBottom)
            {
                lastInView = index;
            }
        }
        ReleaseLazyCardsThrough(lastInView, SettleOrigin::Interaction);
    }

    void EntityPropertyEditor::QueueLazyCardsStep(int delayMs)
    {
        const AZ::u64 generation = m_lazyCardsGeneration;
        QTimer::singleShot(delayMs, this, [this, generation]()
            {
                RunLazyCardsStep(generation);
            });
    }

    void EntityPropertyEditor::RunLazyCardsStep(AZ::u64 generation)
    {
        // A rebuild queued or ran since the cards were held: it holds its own
        if (generation != m_lazyCardsGeneration || m_isAlreadyQueuedRefresh || m_isBuildingProperties)
        {
            return;
        }
        // Not while a card or a row is being dragged around: building moves the cards under it
        if (m_currentReorderState != ReorderState::Inactive)
        {
            QueueLazyCardsStep(50);
            return;
        }

        // The next held cards from the top, for about one step's budget, at least one card; while the inspector keeps its last
        // picture, the cards in view, for the rebuild's budget: nothing shows before they are built
        if (m_viewFrozen)
        {
            BuildViewCards(AZStd::max(InterruptibleInlineBudgetUs(), LazyCardsStepBudgetUs));
        }
        else
        {
            ReleaseLazyCardsThrough(m_componentEditorsUsed - 1, SettleOrigin::Build, LazyCardsStepBudgetUs);
        }
        ++m_lazyCardsSteps;

        // the cards in view built: the frozen inspector shows them
        if (m_viewFrozen && ViewCardsBuilt())
        {
            EndViewFreeze();
        }

        if (GetLazyCardsPending() > 0)
        {
            QueueLazyCardsStep();
        }
        else
        {
            FinishLazyCards();
        }
    }

    void EntityPropertyEditor::CompleteLazyCards()
    {
        ReleaseLazyCardsThrough(m_componentEditorsUsed - 1, SettleOrigin::Interaction);
        FinishLazyCards();
    }

    void EntityPropertyEditor::FinishLazyCards()
    {
        // Every card is built: a freeze cannot outlast it
        EndViewFreeze();

        // The generation ends here: a step still queued for it does nothing
        ++m_lazyCardsGeneration;
        if (!m_lazyCardsDiagnostics)
        {
            return;
        }
        m_lazyCardsDiagnostics = false;

        // The time from the start of the rebuild that held the cards to the last card built, and what the held cards' builds cost
        const AZ::s64 nowUs =
            AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now().time_since_epoch()).count();
        AZ::u64 resetRowsUs = 0;
        AZ::u64 resetDirtyHandlersUs = 0;
        DocumentPropertyEditor::TakeResetTimings(resetRowsUs, resetDirtyHandlersUs);
        const AZ::u64 generateContentsUs = AZ::DocumentPropertyEditor::ReflectionAdapter::TakeGenerateContentsMicroseconds();
        AZ_Printf(
            "EntityInspector",
            "  lazyCards: held=%d steps=%d completeMs=%.2f dom=%.2f rows=%.2f dirtyHandlers=%.2f (ms)",
            m_lazyCardsHeldAtBuild,
            m_lazyCardsSteps,
            aznumeric_cast<double>(nowUs - m_lazyCardsStartUs) / 1000.0,
            aznumeric_cast<double>(generateContentsUs) / 1000.0,
            aznumeric_cast<double>(resetRowsUs) / 1000.0,
            aznumeric_cast<double>(resetDirtyHandlersUs) / 1000.0);
    }

    void EntityPropertyEditor::UpdateLazyCardsPendingProperty()
    {
        // read by the PerfBench kit, which waits for every card's rows before it reads the inspector
        const int pending = GetLazyCardsPending();
        if (property("lazyCardsPending").toInt() != pending)
        {
            setProperty("lazyCardsPending", pending);
        }
    }

    // ----------------------------------------------------------------------------------
    // Interruptible load (ed_inspectorInterruptibleLoad)
    // ----------------------------------------------------------------------------------

    void EntityPropertyEditor::RunQueuedRefresh()
    {
        // A selection queues this rebuild ahead of the paints it causes (Qt posts paints at low priority), so the outliner showed the
        // new selection only once the inspector was rebuilt. The pending paints go first.
        if (ed_inspectorInterruptibleLoad)
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::UpdateRequest);
        }

        m_interruptibleRebuild = ed_inspectorInterruptibleLoad;
        UpdateContents();
        m_interruptibleRebuild = false;
    }

    AZ::s64 EntityPropertyEditor::ShowRebuiltView()
    {
        // An interruptible rebuild builds the cards in view for a while at once, then keeps the inspector's last picture until the idle
        // steps have built the rest of them
        if (m_interruptibleRebuild && InterruptibleInlineBudgetUs() > 0)
        {
            BuildViewCards(InterruptibleInlineBudgetUs());
        }
        if (m_interruptibleRebuild && !ViewCardsBuilt())
        {
            StartViewFreeze();
            return 0;
        }
        setUpdatesEnabled(true);
        return m_interruptibleRebuild
            ? AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now().time_since_epoch()).count()
            : 0;
    }

    int EntityPropertyEditor::FirstViewCardHeld() const
    {
        // the bottom of the view the rebuild restores (the saved scroll position), and the cards' tops at their heights now (a held
        // card has its header's)
        const int viewBottom = AZStd::max(m_savedVerticalScroll, 0) + m_gui->m_componentList->viewport()->height();
        const QLayout* listLayout = m_gui->m_componentListContents->layout();
        const int spacing = AZStd::max(listLayout->spacing(), 0);
        int top = listLayout->contentsMargins().top();
        for (AZ::s32 index = 0; index < m_componentEditorsUsed && top < viewBottom; ++index)
        {
            const ComponentEditor* componentEditor = m_componentEditors[index];
            if (!componentEditor->isVisible())
            {
                continue;
            }
            if (componentEditor->IsContentBuildHeld() && componentEditor->IsExpanded())
            {
                return index;
            }
            top += componentEditor->sizeHint().height() + spacing;
        }
        return -1;
    }

    void EntityPropertyEditor::BuildViewCards(AZ::s64 budgetUs)
    {
        const AZStd::chrono::steady_clock::time_point start = AZStd::chrono::steady_clock::now();
        for (int held = FirstViewCardHeld(); held >= 0; held = FirstViewCardHeld())
        {
            ReleaseLazyCardsThrough(held, SettleOrigin::Build);

            const AZ::s64 elapsedUs =
                AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - start).count();
            if (elapsedUs >= budgetUs)
            {
                return;
            }
        }
    }

    void EntityPropertyEditor::StartViewFreeze()
    {
        // the updates stay disabled (UpdateContents disabled them) and the mouse is ignored until EndViewFreeze
        m_viewFrozen = true;
        const AZ::u64 generation = ++m_viewFreezeGeneration;
        QTimer::singleShot(ViewFreezeMaxMs, this, [this, generation]()
            {
                // the steps did not get there (paused, or a rebuild is queued and will end the freeze): build the view now
                if (generation != m_viewFreezeGeneration || !m_viewFrozen || m_isAlreadyQueuedRefresh || m_isBuildingProperties)
                {
                    return;
                }
                BuildViewCards(AZStd::numeric_limits<AZ::s64>::max());
                EndViewFreeze();
            });
    }

    void EntityPropertyEditor::EndViewFreeze()
    {
        if (!m_viewFrozen)
        {
            return;
        }
        m_viewFrozen = false;

        // The cards in view at their heights before the one paint that shows them (a step leaves the layout to the event loop),
        // and the scroll position restored again: when the rebuild restored it, the held cards were too short to reach it
        SettleComponentList(SettleOrigin::Interaction);
        if (m_savedVerticalScroll >= 0)
        {
            m_gui->m_componentList->verticalScrollBar()->setValue(m_savedVerticalScroll);
        }
        setUpdatesEnabled(true);
        ReportFirstView(
            AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now().time_since_epoch()).count());
    }

    void EntityPropertyEditor::ReportFirstView(AZ::s64 shownUs) const
    {
        if (m_inspectorDiagnosticsActive)
        {
            AZ_Printf(
                "EntityInspector", "  firstView: ms=%.2f steps=%d (ms since the rebuild started)",
                aznumeric_cast<double>(shownUs - m_lazyCardsStartUs) / 1000.0, m_lazyCardsSteps);
        }
    }

    bool EntityPropertyEditor::IgnoredWhileFrozen(const QObject* object, const QEvent* event) const
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
        case QEvent::ContextMenu:
            return object->isWidgetType() && (object == this || isAncestorOf(static_cast<const QWidget*>(object)));
        default:
            return false;
        }
    }
} // namespace AzToolsFramework
