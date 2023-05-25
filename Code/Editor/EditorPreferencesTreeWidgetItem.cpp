/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesTreeWidgetItem.h"

// AzCore
#include <AzCore/Component/ComponentApplicationBus.h>

// AzToolsFramework
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

// Editor
#include "IPreferencesPage.h"

#include <AzQtComponents/Components/StyleManager.h>

EditorPreferencesTreeWidgetItem::EditorPreferencesTreeWidgetItem(IPreferencesPage* page, const QPixmap& selectedImage, QPixmap& unselectedImage)
    : QTreeWidgetItem(EditorPreferencesPage)
    , m_preferencesPage(page)
    , m_selectedImage(selectedImage)
    , m_unselectedImage(unselectedImage)
    , m_entirePageMatchesFilter(true)
{
    Setup(page);
}

EditorPreferencesTreeWidgetItem::EditorPreferencesTreeWidgetItem(IPreferencesPage* page, const QIcon& icon)
    : QTreeWidgetItem(EditorPreferencesPage)
    , m_preferencesPage(page)
    , m_entirePageMatchesFilter(true)
{
    setIcon(0, icon);

    setData(0, Qt::DecorationRole, icon);
    Setup(page);
}

void EditorPreferencesTreeWidgetItem::Setup(IPreferencesPage* page)
{
    setData(0, Qt::DisplayRole, m_preferencesPage->GetTitle());

    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(serializeContext, "Serialization context not available");

    // Grab all property names on the page so we can filter by property text by recursing through the hierarchy
    AzToolsFramework::InstanceDataHierarchy hierarchy;
    hierarchy.AddRootInstance(page);
    hierarchy.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
    std::function<void(AzToolsFramework::InstanceDataNode&)> visitNode = [&](AzToolsFramework::InstanceDataNode& node)
    {
        const AZ::Edit::ElementData* data = node.GetElementEditMetadata();
        QString text;
        if (data)
        {
            text = data->m_name;
        }

        if (!text.isEmpty() && !m_propertyNames.contains(text))
        {
            m_propertyNames.append(text);
        }

        for (AzToolsFramework::InstanceDataNode& child : node.GetChildren())
        {
            visitNode(child);
        }
    };
    visitNode(hierarchy);
}

EditorPreferencesTreeWidgetItem::~EditorPreferencesTreeWidgetItem()
{
}


IPreferencesPage* EditorPreferencesTreeWidgetItem::GetPreferencesPage() const
{
    return m_preferencesPage;
}

void EditorPreferencesTreeWidgetItem::Filter(const QString& filter)
{
    // Everything on a page matches the filter if its or any of its parents' text matches
    m_entirePageMatchesFilter = false;
    QTreeWidgetItem* item = this;
    while (item && !m_entirePageMatchesFilter)
    {
        m_entirePageMatchesFilter = item->text(0).contains(filter, Qt::CaseInsensitive);
        item = item->parent();
    }

    // Otherwise, just check the contents to see if there's a match
    bool filtered = !m_entirePageMatchesFilter;
    for (auto it = m_propertyNames.begin(), end = m_propertyNames.end(); it != end && filtered; ++it)
    {
        filtered = !(*it).contains(filter, Qt::CaseInsensitive);
    }

    setHidden(filtered);
}

void EditorPreferencesTreeWidgetItem::UpdateEditorFilter(AzToolsFramework::ReflectedPropertyEditor* editor, const QString& filter)
{
    QString filterText = m_entirePageMatchesFilter ? QString() : filter;
    editor->InvalidateAll(filterText.toUtf8().constData());
    editor->ExpandAll();
}
