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
#include <QPushButton>
#include <QWidget>
#include <QHBoxLayout>
#include <EventHandlerWidget.h>
#endif

namespace OpenParticleSystemEditor
{
    class EventHandlerWidgets
        : public QWidget
    {
        Q_OBJECT;

    public:
        explicit EventHandlerWidgets(
            AZ::SerializeContext* serializeContext,
            AzToolsFramework::IPropertyEditorNotify* pnotify,
            QWidget* parent = nullptr);
        ~EventHandlerWidgets() override{};

        void ClearEventHandlerWidgets();
        void Clear();
        void AddEventHandler(const AZStd::string& moduleName, AZStd::any* instance);
        void DeleteEventHandler(AZStd::string& moduleName);
        int GetEventEmitterName(size_t index);
        void SetCheckBoxEnable(bool check) const;

    private:
        AZStd::string m_className;
        AZ::SerializeContext* m_serializeContext = nullptr;
        AzToolsFramework::IPropertyEditorNotify* m_pnotify = nullptr;
        QPushButton* m_btnAddEventHandler = nullptr;
        QPushButton* m_btnAddInheritanceEventHandler = nullptr;
        QWidget* m_parent = nullptr;
        QVBoxLayout* m_layout = nullptr;
        AZStd::vector<EventHandlerWidget*> m_eventHandlerWidgets;
    };
}

