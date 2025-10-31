/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>

#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Source/Editor/ObjectEditor.h>
#include <EMotionFX/Source/Event.h>
#endif

class QLabel;
class QMenu;
class QAction;

namespace EMotionFX
{
    class ObjectEditor;
    class MotionEvent;
}

namespace EMStudio
{
    class EventDataEditor;

    class EventDataPropertyNotify
        : public AzToolsFramework::IPropertyEditorNotify
    {
    public:
        EventDataPropertyNotify(const EventDataEditor* editor);
        virtual ~EventDataPropertyNotify() = default;

        // this function gets called each time you are about to actually modify a property (not when the editor opens)
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode*) override {}

        // this function gets called each time a property is actually modified (not just when the editor appears),
        // for each and every change - so for example, as a slider moves.
        // its meant for undo state capture.
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode*) override {}

        // this funciton is called when some stateful operation begins, such as dragging starts in the world editor or such
        // in which case you don't want to blow away the tree and rebuild it until editing is complete since doing so is
        // flickery and intensive.
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode*) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;

        // this will cause the current undo operation to complete, sealing it and beginning a new one if there are further edits.
        void SealUndoStack() override {}

    private:
        const EventDataEditor* m_editor;
    };


    class EventDataEditor
        : public QWidget
    {
        Q_OBJECT
    public:
        EventDataEditor(EMotionFX::Motion* motion, EMotionFX::MotionEvent* event, EMotionFX::EventDataSet* eventDataSet, QWidget* parent = nullptr);

        void SetEventDataSet(EMotionFX::Motion* motion, EMotionFX::MotionEvent* event, EMotionFX::EventDataSet* eventDataSet);
        void MoveEventDataSet(EMotionFX::EventDataSet& targetDataSet);

        void AppendEventData(const AZ::Uuid& newTypeId);
        void RemoveEventData(size_t index);
        AZ::Outcome<size_t> FindEventDataIndex(const EMotionFX::EventData* eventData) const;

        EMotionFX::Motion* GetMotion() const;
        EMotionFX::MotionEvent* GetMotionEvent() const;

    Q_SIGNALS:
        void eventsChanged(EMotionFX::Motion*, EMotionFX::MotionEvent*);

    private:
        void Init();

        AzQtComponents::Card* AppendCard(AZ::SerializeContext* context, size_t cardIndex);

        EMotionFX::ObjectEditor* GetObjectEditorFromCard(const AzQtComponents::Card* card);

        AZStd::vector<AZStd::unique_ptr<EMotionFX::EventData>> m_eventDataSet;
        AzQtComponents::Card* m_topLevelEventDataCard = nullptr;
        QWidget* m_eventDataCardsContainer = nullptr;
        QMenu* m_eventDataSelectionMenu = nullptr;
        QMenu* m_deleteCurrentEventDataMenu = nullptr;
        QAction* m_deleteAction = nullptr;
        QLabel* m_emptyLabel = nullptr;
        EMotionFX::Motion* m_motion = nullptr;
        EMotionFX::MotionEvent* m_motionEvent = nullptr;
        AZStd::unique_ptr<EventDataPropertyNotify> m_propertyNotify;
        AZStd::vector<AzQtComponents::Card*> m_eventDataCards;
    };
} // end namespace EMStudio
