/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ObjectEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QVBoxLayout>


namespace EMotionFX
{
    const int ObjectEditor::s_propertyLabelWidth = 160;

    ObjectEditor::ObjectEditor(AZ::SerializeContext* serializeContext, QWidget* parent)
        : ObjectEditor(serializeContext, nullptr, parent)
    {
    }

    ObjectEditor::ObjectEditor(AZ::SerializeContext* serializeContext, AzToolsFramework::IPropertyEditorNotify* notify, QWidget* parent)
        : QFrame(parent)
        , m_object(nullptr)
        , m_propertyEditor(nullptr)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_propertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_propertyEditor->setObjectName("PropertyEditor");
        m_propertyEditor->Setup(serializeContext, notify, false/*enableScrollbars*/, s_propertyLabelWidth);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addWidget(m_propertyEditor);
        setLayout(mainLayout);
    }


    void ObjectEditor::AddInstance(void* object, const AZ::TypeId& objectTypeId, void* aggregateInstance, void* compareInstance)
    {
        m_object = object;
        m_propertyEditor->AddInstance(object, objectTypeId, aggregateInstance, compareInstance);
        m_propertyEditor->InvalidateAll();
    }


    void ObjectEditor::ClearInstances(bool invalidateImmediately)
    {
        m_propertyEditor->ClearInstances();
        if (invalidateImmediately)
        {
            m_propertyEditor->InvalidateAll();
        }
        m_object = nullptr;
    }


    void ObjectEditor::InvalidateAll()
    {
        m_propertyEditor->InvalidateAll();
    }


    void ObjectEditor::InvalidateValues()
    {
        m_propertyEditor->InvalidateValues();
    }

} // namespace EMotionFX
