/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxMaterialUiUnlock.h"

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <Components/EditorWhiteBoxColliderComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>

#include <QApplication>
#include <QEvent>
#include <QGraphicsEffect>
#include <QVariant>
#include <QWidget>

namespace WhiteBox
{
    namespace
    {
        // Set by AzQtComponents::SetWidgetInteractEnabled on the dimming effect it adds.
        constexpr const char* DisableEffectProperty = "QtViewPaneManagerDisableEffect";

        bool IsMaterialUi(QWidget* widget)
        {
            if (widget->inherits("AZ::Render::EditorMaterialComponentInspector::MaterialPropertyInspector"))
            {
                return true;
            }
            if (!widget->inherits("AzToolsFramework::ComponentEditor"))
            {
                return false;
            }
            const AZ::Uuid& type = static_cast<AzToolsFramework::ComponentEditor*>(widget)->GetComponentType();
            return type == AZ::Render::EditorMaterialComponentTypeId || type == azrtti_typeid<EditorWhiteBoxColliderComponent>();
        }
    } // namespace

    WhiteBoxMaterialUiUnlock::WhiteBoxMaterialUiUnlock()
    {
        // The inspector and the pane manager disable things as component mode starts; run after them.
        QueueUnlock();
        m_rescan.setInterval(1000);
        connect(&m_rescan, &QTimer::timeout, this, [this] { Unlock(); });
        m_rescan.start();
    }

    void WhiteBoxMaterialUiUnlock::QueueUnlock()
    {
        if (!m_queued)
        {
            m_queued = true;
            QTimer::singleShot(0, this, [this] { Unlock(); });
        }
    }

    void WhiteBoxMaterialUiUnlock::Unlock()
    {
        m_queued = false;
        for (QWidget* widget : QApplication::allWidgets())
        {
            if (!IsMaterialUi(widget))
            {
                continue;
            }
            if (!m_watched.contains(widget))
            {
                m_watched.insert(widget);
                widget->installEventFilter(this); // removed by Qt when this object goes
                connect(widget, &QObject::destroyed, this, [this](QObject* gone) { m_watched.remove(gone); });
            }
            if (!widget->isEnabled())
            {
                widget->setEnabled(true);
            }
            if (QGraphicsEffect* effect = widget->graphicsEffect(); effect && !effect->property(DisableEffectProperty).isNull())
            {
                widget->setGraphicsEffect(nullptr);
            }
        }
    }

    bool WhiteBoxMaterialUiUnlock::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::EnabledChange)
        {
            if (auto* widget = qobject_cast<QWidget*>(watched); widget && !widget->isEnabled())
            {
                QueueUnlock();
            }
        }
        return QObject::eventFilter(watched, event);
    }
} // namespace WhiteBox
