/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"
#include "AssetDropHelpers.h"

#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDropEvent>
#include <QDragEnterEvent>

PropertiesWrapper::PropertiesWrapper(HierarchyWidget* hierarchy,
    EditorWindow* parent)
    : QWidget(parent)
    , m_editorWindow(parent)
    , m_properties(new PropertiesWidget(parent, this))
    , m_componentButton(new ComponentButton(hierarchy, this))
{
    AZ_Assert(parent, "Parent EditorWindow is null");

    setAcceptDrops(true);

    QVBoxLayout* outerLayout = new QVBoxLayout(this);

    QVBoxLayout* innerLayout = new QVBoxLayout();
    {
        innerLayout->setContentsMargins(4, 4, 4, 4);
        innerLayout->setSpacing(4);

        QHBoxLayout* innerHLayout = new QHBoxLayout();
        {
            QLabel* elementNameLabel = new QLabel(this);
            elementNameLabel->setText("Name");
            innerHLayout->addWidget(elementNameLabel);
            QLineEdit* elementNameLineEdit = new QLineEdit(this);
            elementNameLineEdit->setObjectName(QStringLiteral("m_elementName"));
            elementNameLineEdit->setText("No Canvas Loaded");
            innerHLayout->addWidget(elementNameLineEdit);
            m_properties->SetSelectedEntityDisplayNameWidget(elementNameLineEdit);
        }
        innerLayout->addLayout(innerHLayout);

        m_editorOnlyCheckbox = new QCheckBox("Editor Only");
        m_editorOnlyCheckbox->setVisible(false);
        innerLayout->addWidget(m_editorOnlyCheckbox, 0, Qt::AlignCenter);
        m_properties->SetEditorOnlyCheckbox(m_editorOnlyCheckbox);
    }
    outerLayout->addLayout(innerLayout);

    outerLayout->addWidget(m_componentButton);

    outerLayout->addWidget(m_properties);

    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    m_properties->setEnabled(false);
    m_componentButton->setEnabled(false);
}

PropertiesWidget* PropertiesWrapper::GetProperties()
{
    return m_properties;
}

void PropertiesWrapper::ActiveCanvasChanged()
{
    // Entity shown in the properties pane has been deleted and a new entity is selected, so trigger an immediate refresh
    m_properties->TriggerImmediateRefresh();

    bool canvasLoaded = m_editorWindow->GetCanvas().IsValid();
    m_properties->setEnabled(canvasLoaded);
    m_componentButton->setEnabled(canvasLoaded);
}

bool PropertiesWrapper::AcceptsMimeData(const QMimeData* mimeData) const
{
    bool canvasLoaded = m_editorWindow->GetCanvas().IsValid();
    if (!canvasLoaded)
    {
        return false;
    }

    if (!AssetDropHelpers::AcceptsMimeType(mimeData))
    {
        return false;
    }

    ComponentAssetHelpers::ComponentAssetPairs componentAssetPairs;
    AssetDropHelpers::AssetList sliceAssets;
    AssetDropHelpers::DecodeSliceAndComponentAssetsFromMimeData(mimeData, componentAssetPairs, sliceAssets);

    if (componentAssetPairs.empty())
    {
        return false;
    }

    AZStd::vector<AZ::TypeId> componentTypes;
    componentTypes.reserve(componentAssetPairs.size());
    for (const ComponentAssetHelpers::ComponentAssetPair& pair : componentAssetPairs)
    {
        componentTypes.push_back(pair.first);
    }
    return ComponentHelpers::CanAddComponentsToSelectedEntities(componentTypes);
}

void PropertiesWrapper::dragEnterEvent(QDragEnterEvent* event)
{
    if (AcceptsMimeData(event->mimeData()))
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void PropertiesWrapper::dropEvent(QDropEvent* event)
{
    if (AcceptsMimeData(event->mimeData()))
    {
        DropMimeDataAssets(event->mimeData());
        event->acceptProposedAction();

        // Put focus on the properties widget
        activateWindow();
        setFocus();
    }
}

void PropertiesWrapper::DropMimeDataAssets(const QMimeData* mimeData)
{
    ComponentAssetHelpers::ComponentAssetPairs componentAssetPairs;
    AssetDropHelpers::AssetList sliceAssets;
    AssetDropHelpers::DecodeSliceAndComponentAssetsFromMimeData(mimeData, componentAssetPairs, sliceAssets);

    ComponentHelpers::AddComponentsWithAssetToSelectedEntities(componentAssetPairs);
}

#include <moc_PropertiesWrapper.cpp>
