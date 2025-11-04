/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <QToolButton>
#include <QWidget>
#include <QHBoxLayout>
#endif

namespace OpenParticleSystemEditor
{
    class EventHandlerWidget
        : public QWidget
    {
        Q_OBJECT;

    public:
        EventHandlerWidget(
            const AZStd::string& moduleName,
            AZStd::any* instance,
            AZ::SerializeContext* serializeContext,
            AzToolsFramework::IPropertyEditorNotify* pnotify,
            QWidget* parent = nullptr);
        ~EventHandlerWidget() override{};

        void Refresh() const;
        void ClearInstances();
        int GetEventEmitterName(size_t index);
        AZ::TypeId m_moduleId;

    private slots:
        void OnClickDelete();

    private:
        AZStd::string m_moduleName;
        AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
        QToolButton* m_btnDelete;
        QHBoxLayout* m_layout;
        QWidget* m_parent;
    };
}

