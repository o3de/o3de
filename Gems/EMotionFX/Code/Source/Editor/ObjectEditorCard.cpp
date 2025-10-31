/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <Editor/ObjectEditorCard.h>
#include <QWidget>

namespace EMStudio
{
    ObjectEditorCard::ObjectEditorCard(AZ::SerializeContext* serializeContext, QWidget* parent)
        : AzQtComponents::Card(parent)
    {
        m_objectEditor = new EMotionFX::ObjectEditor(serializeContext, this);
        m_objectEditor->setObjectName("EMFX.AttributesWindow.ObjectEditor");
        setContentWidget(m_objectEditor);
    }

    void ObjectEditorCard::Update(const QString& cardName, const AZ::TypeId& objectTypeId, void* object)
    {
        setTitle(cardName);
        setExpanded(true);

        AzQtComponents::CardHeader* cardHeader = header();
        cardHeader->setHasContextMenu(false);
        cardHeader->setHelpURL("");

        m_objectEditor->ClearInstances(true);
        m_objectEditor->AddInstance(object, objectTypeId);
    }
} // namespace EMStudio
