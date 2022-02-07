/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LibraryDataModel.h"

#include <QIcon>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <Libraries/Libraries.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ScriptCanvasEditor
{
    namespace Model
    {
        LibraryData::LibraryData(QObject* parent /*= nullptr*/) : QAbstractTableModel(parent)
        {
            Add("All", AZ::Uuid::CreateNull());

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            serializeContext->EnumerateDerived<ScriptCanvas::Library::LibraryDefinition>(
                [this]
            (const AZ::SerializeContext::ClassData* classData, [[maybe_unused]] const AZ::Uuid& classUuid) -> bool
            {
                Add(classData->m_name, classData->m_typeId);
                return true;
            });
        }

        int LibraryData::rowCount([[maybe_unused]] const QModelIndex &parent /*= QModelIndex()*/) const
        {
            return m_data.size();
        }

        int LibraryData::columnCount([[maybe_unused]] const QModelIndex &parent /*= QModelIndex()*/) const
        {
            return ColumnIndex::Count;
        }

        QVariant LibraryData::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
        {
            switch (role)
            {
            case DataSetRole:
            {
                if (index.column() == ColumnIndex::Name)
                {
                    const Data* data = &m_data[index.row()];
                    return QVariant::fromValue<void*>(reinterpret_cast<void*>(const_cast<Data*>(data)));
                }
            }
            break;

            case Qt::DisplayRole:
            {
                if (index.column() == ColumnIndex::Name)
                {
                    return m_data[index.row()].m_name;
                }
            }
            break;

            case Qt::DecorationRole:
            {
                AZ::SerializeContext* serializeContext = nullptr;
                EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(m_data[index.row()].m_uuid);

                if (classData && classData->m_editData)
                {
                    const auto& editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                    if (editorElementData)
                    {
                        if (auto iconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Icon))
                        {
                            if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
                            {
                                AZStd::string iconAttributeValue = iconAttributeData->Get(nullptr);
                                if (!iconAttributeValue.empty())
                                {
                                    return QVariant(QIcon(QString(iconAttributeValue.c_str())));
                                }
                            }
                        }
                    }
                }
                else
                {
                    QString defaultIcon = QStringLiteral("Icons/ScriptCanvas/Libraries/All.png");
                    return QVariant(QIcon(defaultIcon));
                }

                return QVariant();
            }
            break;

            default:
                break;
            }

            return QVariant();
        }

        Qt::ItemFlags LibraryData::flags([[maybe_unused]] const QModelIndex &index) const
        {
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
        }

        void LibraryData::Add(const char* name, const AZ::Uuid& uuid)
        {
            m_data.push_back({ QString(name), uuid });
        }

    }
}

