/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/QtMetaTypes.h>
#include <Editor/PropertyWidgets/EventDataHandler.h>
#include <QAbstractListModel>
#include <QVBoxLayout>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EventDataTypeSelectionWidget, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(EventDataHandler, EditorAllocator)

    class EventDataTypeSelectionWidget::EventDataTypesModel
        : public QAbstractListModel
    {
    public:
        enum
        {
            UuidRole = Qt::UserRole
        };

        EventDataTypesModel();
        EventDataTypesModel(const EventDataTypesModel&) = delete;
        EventDataTypesModel(EventDataTypesModel&&) = delete;

        static AZStd::shared_ptr<EventDataTypesModel> Get();

        int rowCount(const QModelIndex& parent) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    private:
        static AZStd::weak_ptr<EventDataTypesModel> dataTypesModel;
        AZStd::vector<AZStd::pair<QString, AZ::Uuid> > m_eventDataClassNames;
    };
    AZStd::weak_ptr<EventDataTypeSelectionWidget::EventDataTypesModel> EventDataTypeSelectionWidget::EventDataTypesModel::dataTypesModel;

    EventDataTypeSelectionWidget::EventDataTypesModel::EventDataTypesModel()
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_eventDataClassNames.emplace_back("<none>", AZ::Uuid::CreateNull());

        context->EnumerateDerived<EventData>(
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
                        const AZ::AttributeContainerType<bool>* typedAttribute = static_cast<const AZ::AttributeContainerType<bool>*>(attribute);
                        // instance is nullptr because this is a class-level
                        // attribute and not one on a specific instance
                        if (typedAttribute->Get(/*instance =*/ nullptr))
                        {
                            m_eventDataClassNames.emplace_back(editData->m_name, classData->m_typeId);
                            break;
                        }
                    }
                }
                return true;
            }
            );
    }

    AZStd::shared_ptr<EventDataTypeSelectionWidget::EventDataTypesModel> EventDataTypeSelectionWidget::EventDataTypesModel::Get()
    {
        AZStd::shared_ptr<EventDataTypesModel> model;
        if (dataTypesModel.expired())
        {
            model = AZStd::make_shared<EventDataTypesModel>();
        }
        else
        {
            model = dataTypesModel.lock();
        }
        return model;
    }

    int EventDataTypeSelectionWidget::EventDataTypesModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            return 0;
        }
        return static_cast<int>(m_eventDataClassNames.size());
    }

    QVariant EventDataTypeSelectionWidget::EventDataTypesModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            return m_eventDataClassNames[index.row()].first;
        }
        else if (role == UuidRole)
        {
            return QVariant::fromValue(m_eventDataClassNames[index.row()].second);
        }
        return QVariant();
    }

    EventDataTypeSelectionWidget::EventDataTypeSelectionWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_comboBox = new QComboBox(this);
        m_model = EventDataTypesModel::Get();
        m_comboBox->setModel(m_model.get());

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(m_comboBox);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        connect(
            m_comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EventDataTypeSelectionWidget::currentIndexChanged
            );
    }

    const AZ::Uuid EventDataTypeSelectionWidget::GetSelectedClass() const
    {
        return m_comboBox->currentData(EventDataTypesModel::UuidRole).value<AZ::Uuid>();
    }

    void EventDataTypeSelectionWidget::SetSelectedClass(const AZ::Uuid& classId)
    {
        const int newRow = m_comboBox->findData(
                QVariant::fromValue(classId),
                EventDataTypesModel::UuidRole,
                Qt::MatchExactly
                );
        m_comboBox->setCurrentIndex(newRow);
    }

    AZ::u32 EventDataHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("EMotionFX::EventData");
    }

    QWidget* EventDataHandler::CreateGUI(QWidget* parent)
    {
        EventDataTypeSelectionWidget* widget = aznew EventDataTypeSelectionWidget(parent);
        connect(widget, &EventDataTypeSelectionWidget::currentIndexChanged, this, [widget]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, widget);
            });
        return widget;
    }

    void EventDataHandler::ConsumeAttribute([[maybe_unused]] EventDataTypeSelectionWidget* widget, [[maybe_unused]] AZ::u32 attrib, [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
    }

    void EventDataHandler::WriteGUIValuesIntoProperty(size_t /*index*/, EventDataTypeSelectionWidget* selectionWidget, AZStd::shared_ptr<const EventData>& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        const AZ::Uuid& newTypeId = selectionWidget->GetSelectedClass();
        if (azrtti_typeid(instance.get()) == newTypeId)
        {
            return;
        }

        if (newTypeId == AZ::Uuid::CreateNull())
        {
            instance = nullptr;
        }
        else
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            const AZ::SerializeContext::ClassData* classData = context->FindClassData(newTypeId);
            instance.reset(static_cast<EMotionFX::EventData*>(classData->m_factory->Create(classData->m_name)));

            // Deduplicate the event data
            instance = GetEMotionFX().GetEventManager()->FindEventData(instance);
        }

        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestRefresh, AzToolsFramework::Refresh_EntireTree);
    }

    bool EventDataHandler::ReadValuesIntoGUI(size_t /*index*/, EventDataTypeSelectionWidget* selectionWidget, const AZStd::shared_ptr<const EventData>& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        if (instance)
        {
            selectionWidget->SetSelectedClass(azrtti_typeid(instance.get()));
        }

        return true;
    }
} // namespace EMotionFX
#include <Source/Editor/PropertyWidgets/moc_EventDataHandler.cpp>
