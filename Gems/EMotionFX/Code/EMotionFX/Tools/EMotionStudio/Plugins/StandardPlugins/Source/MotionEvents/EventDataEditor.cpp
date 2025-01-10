/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>

#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/EventDataEditor.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventEditor.h>
#include <Source/Editor/ObjectEditor.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <MCore/Source/ReflectionSerializer.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

namespace EMStudio
{
    EventDataPropertyNotify::EventDataPropertyNotify(const EventDataEditor* editor)
        : IPropertyEditorNotify()
        , m_editor(editor)
    {
    }

    void EventDataPropertyNotify::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        if (!m_editor->GetMotionEvent())
        {
            return;
        }

        auto parent = pNode->GetParent();
        while (parent)
        {
            // Keep checking the parent node because we could be editing data in deeper layer.
            // For example, eventData holding an array of elements, and we are editing the individual element.
            if (parent->GetSerializeContext()->CanDowncast(
                    parent->GetClassMetadata()->m_typeId,
                    azrtti_typeid<EMotionFX::EventData>(),
                    parent->GetClassMetadata()->m_azRtti,
                    nullptr))
            {
                const EMotionFX::EventData* eventData = static_cast<EMotionFX::EventData*>(parent->FirstInstance());
                const AZ::Outcome<size_t> eventDataIndex = m_editor->FindEventDataIndex(eventData);
                if (eventDataIndex.IsSuccess())
                {
                    CommandSystem::CommandAdjustMotionEvent* adjustMotionEventCommand = aznew CommandSystem::CommandAdjustMotionEvent();
                    adjustMotionEventCommand->SetMotionID(m_editor->GetMotion()->GetID());
                    adjustMotionEventCommand->SetMotionEvent(m_editor->GetMotionEvent());
                    adjustMotionEventCommand->SetEventDataNr(eventDataIndex.GetValue());
                    adjustMotionEventCommand->SetEventData(EMotionFX::EventDataPtr(MCore::ReflectionSerializer::Clone(eventData)));
                    adjustMotionEventCommand->SetEventDataAction(CommandSystem::CommandAdjustMotionEvent::EventDataAction::Replace);

                    AZStd::string result;
                    CommandSystem::GetCommandManager()->ExecuteCommand(adjustMotionEventCommand, result);
                    break;
                }
            }
            parent = parent->GetParent();
        }
    }

    EventDataEditor::EventDataEditor(EMotionFX::Motion* motion, EMotionFX::MotionEvent* event, EMotionFX::EventDataSet* eventDataSet, QWidget* parent)
        : QWidget(parent)
        , m_propertyNotify(AZStd::make_unique<EventDataPropertyNotify>(this))
    {
        Init();
        SetEventDataSet(motion, event, eventDataSet);
    }

    void EventDataEditor::Init()
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_eventDataSelectionMenu = new QMenu(this);
        // Populate the event data selection menu with derived classes of
        // EMotionFX::EventData
        context->EnumerateDerived<EMotionFX::EventData>(
            [this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&)
            {
                // Only add this type if it has the "Creatable" class attribute
                // set to true
                const AZ::Edit::ClassData* editData = classData->m_editData;
                if (!editData)
                {
                    return true;
                }
                for (const AZ::Edit::ElementData& element : editData->m_elements)
                {
                    if (element.m_elementId == AZ::Edit::ClassElements::EditorData)
                    {
                        const AZ::Attribute* attribute = AZ::FindAttribute(AZ_CRC_CE("Creatable"), element.m_attributes);
                        if (!attribute)
                        {
                            continue;
                        }
                        const AZ::AttributeContainerType<bool>* typedAttribute =
                            static_cast<const AZ::AttributeContainerType<bool>*>(attribute);
                        // instance is nullptr because this is a class-level
                        // attribute and not one on a specific instance
                        if (typedAttribute->Get(/*instance =*/nullptr))
                        {
                            this->m_eventDataSelectionMenu->addAction(
                                editData->m_name,
                                this,
                                [this, classData]
                                {
                                    AppendEventData(classData->m_typeId);
                                    emit eventsChanged(GetMotion(), GetMotionEvent());
                                });
                            break;
                        }
                    }
                }
                return true;
            }
        );

        m_deleteCurrentEventDataMenu = new QMenu(this);
        m_deleteAction = m_deleteCurrentEventDataMenu->addAction(
            "Delete",
            this,
            [this]()
            {
                RemoveEventData(m_deleteAction->data().value<size_t>());
                emit eventsChanged(GetMotion(), GetMotionEvent());
            });

        m_emptyLabel = new QLabel("<i>No event data added</i>");

        m_eventDataCardsContainer = new QWidget();
        QVBoxLayout* eventDataCardsLayout = new QVBoxLayout(m_eventDataCardsContainer);
        eventDataCardsLayout->setMargin(0);
        eventDataCardsLayout->setContentsMargins(20, 0, 0, 0);
        eventDataCardsLayout->addWidget(m_emptyLabel);

        m_topLevelEventDataCard = new AzQtComponents::Card();
        m_topLevelEventDataCard->header()->setContextMenuIcon(AzQtComponents::CardHeader::ContextMenuIcon::Plus);
        m_topLevelEventDataCard->setTitle("Event Data");
        m_topLevelEventDataCard->hideFrame();
        m_topLevelEventDataCard->setContentWidget(m_eventDataCardsContainer);
        m_topLevelEventDataCard->connect(m_topLevelEventDataCard, &AzQtComponents::Card::contextMenuRequested, this, [this](const QPoint& pos) { m_eventDataSelectionMenu->exec(pos); });

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_topLevelEventDataCard);
    }

    AzQtComponents::Card* EventDataEditor::AppendCard(AZ::SerializeContext* context, size_t cardIndex)
    {
         AzQtComponents::Card* card = new AzQtComponents::Card();
         card->setContentWidget(new EMotionFX::ObjectEditor(context, m_propertyNotify.get()));
         card->connect(card, &AzQtComponents::Card::contextMenuRequested, this, [this, cardIndex](const QPoint& pos)
         {
             this->m_deleteAction->setData(QVariant::fromValue(cardIndex));
             this->m_deleteCurrentEventDataMenu->exec(pos);
         });
         m_eventDataCards.emplace_back(card);
         m_eventDataCardsContainer->layout()->addWidget(card);

         return card;
    }

    void EventDataEditor::SetEventDataSet(EMotionFX::Motion* motion, EMotionFX::MotionEvent* event, EMotionFX::EventDataSet* eventDataSet)
    {
        m_motion = motion;
        m_motionEvent = event;

        const size_t newEventDataCount = eventDataSet ? eventDataSet->size() : 0;
        AZStd::vector<bool> didEventDataChangeType(newEventDataCount);
        if (eventDataSet)
        {
            m_eventDataSet.resize(newEventDataCount);
            for (size_t i = 0; i < newEventDataCount; ++i)
            {
                const EMotionFX::EventData* eventData = (*eventDataSet)[i].get();
                if (m_eventDataSet[i] && azrtti_typeid(m_eventDataSet[i].get()) == azrtti_typeid(eventData))
                {
                    didEventDataChangeType[i] = false;
                    MCore::ReflectionSerializer::CloneInplace(*m_eventDataSet[i].get(), eventData);
                }
                else
                {
                    didEventDataChangeType[i] = true;
                    m_eventDataSet[i].reset(MCore::ReflectionSerializer::Clone(eventData));
                }
            }
        }
        else
        {
            m_eventDataSet.resize(0);
        }

        m_emptyLabel->setVisible(!bool(newEventDataCount));

        AZ::SerializeContext* context = nullptr;
        if (newEventDataCount)
        {
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        }

        const size_t currentEventDataCount = m_eventDataCards.size();
        if (newEventDataCount > currentEventDataCount)
        {
            // Add cards to the end
            m_eventDataCards.reserve(newEventDataCount);
            for (size_t i = currentEventDataCount; i < newEventDataCount; ++i)
            {
                AppendCard(context, i);
            }
            
        }
        else if (newEventDataCount < currentEventDataCount)
        {
            // Remove cards from the end
            for (size_t i = newEventDataCount; i < currentEventDataCount; ++i)
            {
                delete m_eventDataCards[i];
            }
            m_eventDataCards.resize(newEventDataCount);
        }

        for (size_t i = 0; i < newEventDataCount; ++i)
        {
            AzQtComponents::Card* card = m_eventDataCards[i];
            EMotionFX::ObjectEditor* eventDataEditor = GetObjectEditorFromCard(card);
            auto& eventData = m_eventDataSet[i];

            if (didEventDataChangeType[i])
            {
                eventDataEditor->ClearInstances(false);
                eventDataEditor->AddInstance(eventData.get(), azrtti_typeid(eventData.get()));
                card->setTitle(eventData->RTTI_GetTypeName());
            }
            else
            {
                eventDataEditor->InvalidateValues();
            }
        }
    }

    void EventDataEditor::MoveEventDataSet(EMotionFX::EventDataSet& targetDataSet)
    {
        targetDataSet.resize(m_eventDataSet.size());
        const size_t eventDataCount = m_eventDataSet.size();
        for (size_t i = 0; i < eventDataCount; ++i)
        {
            targetDataSet[i] = AZStd::move(m_eventDataSet[i]);

            delete m_eventDataCards[i];
        }
        m_eventDataCards.resize(0);
        m_eventDataSet.resize(0);
    }

    void EventDataEditor::AppendEventData(const AZ::Uuid& newTypeId)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(newTypeId);

        EMotionFX::EventData* newData {static_cast<EMotionFX::EventData*>(classData->m_factory->Create(classData->m_name))};

        if (GetMotion() && GetMotionEvent())
        {
            // This editor is connected to a Motion and Motion Event. Issue an
            // AdjustMotionEvent command to store undo state.
            CommandSystem::CommandAdjustMotionEvent* adjustMotionEventCommand = aznew CommandSystem::CommandAdjustMotionEvent();
            adjustMotionEventCommand->SetMotionID(GetMotion()->GetID());
            adjustMotionEventCommand->SetMotionEvent(GetMotionEvent());
            adjustMotionEventCommand->SetEventDataAction(CommandSystem::CommandAdjustMotionEvent::EventDataAction::Add);
            adjustMotionEventCommand->SetEventData(EMotionFX::EventDataPtr(newData));

            AZStd::string result;
            CommandSystem::GetCommandManager()->ExecuteCommand(adjustMotionEventCommand, result);
        }
        else
        {
            // This editor is not connected to a Motion Event, but rather a
            // Motion Event Preset. Just update our internal state.
            m_eventDataSet.emplace_back(newData);

            m_emptyLabel->setVisible(m_eventDataSet.empty());

            AzQtComponents::Card* card = AppendCard(context, m_eventDataSet.size() - 1);
            card->setTitle(newData->RTTI_GetTypeName());

            EMotionFX::ObjectEditor* eventDataEditor = GetObjectEditorFromCard(card);
            eventDataEditor->AddInstance(newData, azrtti_typeid(newData));
        }
    }

    void EventDataEditor::RemoveEventData(size_t index)
    {
        if (GetMotion() && GetMotionEvent())
        {
            CommandSystem::CommandAdjustMotionEvent* adjustMotionEventCommand = aznew CommandSystem::CommandAdjustMotionEvent();
            adjustMotionEventCommand->SetMotionID(GetMotion()->GetID());
            adjustMotionEventCommand->SetMotionEvent(GetMotionEvent());
            adjustMotionEventCommand->SetEventDataNr(index);
            adjustMotionEventCommand->SetEventDataAction(CommandSystem::CommandAdjustMotionEvent::EventDataAction::Remove);

            AZStd::string result;
            CommandSystem::GetCommandManager()->ExecuteCommand(adjustMotionEventCommand, result);
        }
        else
        {
            // Each card stores the event data index that it will remove as a
            // constant. Delete the last card and then update the objects
            // displayed in each RPE.
            delete m_eventDataCards[m_eventDataCards.size() - 1];
            m_eventDataCards.erase(m_eventDataCards.end() - 1);

            m_eventDataSet.erase(AZStd::next(m_eventDataSet.begin(), index));
            m_emptyLabel->setVisible(m_eventDataSet.empty());

            const size_t newEventDataCount = m_eventDataCards.size();
            for (size_t i = index; i < newEventDataCount; ++i)
            {
                AzQtComponents::Card* card = m_eventDataCards[i];
                EMotionFX::ObjectEditor* eventDataEditor = GetObjectEditorFromCard(card);
                auto& eventData = m_eventDataSet[i];

                eventDataEditor->ClearInstances(false);
                eventDataEditor->AddInstance(eventData.get(), azrtti_typeid(eventData.get()));
                card->setTitle(eventData->RTTI_GetTypeName());
            }
        }
    }

    AZ::Outcome<size_t> EventDataEditor::FindEventDataIndex(const EMotionFX::EventData* eventData) const
    {
        const auto found = AZStd::find_if(m_eventDataSet.begin(), m_eventDataSet.end(), [eventData](const auto& data) { return data.get() == eventData; });
        if (found != m_eventDataSet.end())
        {
            return AZ::Success(static_cast<size_t>(AZStd::distance(m_eventDataSet.begin(), found)));
        }
        return AZ::Failure();
    }

    EMotionFX::Motion* EventDataEditor::GetMotion() const
    {
        return m_motion;
    }

    EMotionFX::MotionEvent* EventDataEditor::GetMotionEvent() const
    {
        return m_motionEvent;
    }

    EMotionFX::ObjectEditor* EventDataEditor::GetObjectEditorFromCard(const AzQtComponents::Card* card)
    {
        return static_cast<EMotionFX::ObjectEditor*>(card->contentWidget());
    }
} // end namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/moc_EventDataEditor.cpp>
