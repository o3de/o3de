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
#include <QCheckBox>
#include <QWidget>
#include <QHBoxLayout>
#endif

namespace OpenParticleSystemEditor
{
    class PropertyEditorWidget
        : public QWidget
    {
        Q_OBJECT;

    public:
        PropertyEditorWidget(
            const AZStd::string& moduleName,
            const AZStd::string& className,
            bool checked,
            AZStd::any* instance,
            AZ::SerializeContext* serializeContext,
            AzToolsFramework::IPropertyEditorNotify* pnotify,
            QWidget* parent = nullptr);
        ~PropertyEditorWidget() override {};

        void Refresh() const;
        void Refresh(AZStd::any* instance);
        void ClearInstances();
        AZStd::string GetModuleName();
        int GetEventEmitterName(size_t index);
        void SetCheckBoxEnable(bool check) const;
        void SetChecked(bool ckecked) const;

    private slots:
        void OnClickCheckBox(bool bCheck);

    private:
        AZStd::string m_moduleName;
        AZStd::string m_className;
        AZStd::string m_widgetName;
        AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
        QCheckBox* m_checkBox;
        QHBoxLayout* m_layout;
        QWidget* m_parent;
        AZStd::any* m_instance;
    };
}

