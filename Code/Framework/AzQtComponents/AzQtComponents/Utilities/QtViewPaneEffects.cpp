/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtViewPaneEffects.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QGraphicsEffect>
#include <QWidget>
#include <QVariant>

namespace AzQtComponents
{
    static const char* s_viewPaneManagerDisableEffectPropertyName = "QtViewPaneManagerDisableEffect";
    static const float s_disabledPaneOpacity = 0.4f;

    // put the widget into a grayed out state to indicate it cannot be interacted with
    static void DisableViewPaneDisabledGraphicsEffect(QWidget* widget)
    {
        // only remove effects from widgets that we added them to
        QGraphicsEffect* effect = widget->graphicsEffect();
        if (effect != nullptr && !effect->property(s_viewPaneManagerDisableEffectPropertyName).isNull())
        {
            widget->setGraphicsEffect(nullptr);
        }
    }

    // force the widget to be repainted
    static void ForceWidgetRedraw(QWidget* widget)
    {
        widget->hide();
        widget->resize(widget->size());
        widget->show();
    }

    // restore the widget to its normal state to show it can now be interacted with
    static void EnableViewPaneDisabledGraphicsEffect(QWidget* widget)
    {
        // only apply effects to widgets that didn't have them already...
        if (widget->graphicsEffect() == nullptr)
        {
            auto effect = AZStd::make_unique<QGraphicsOpacityEffect>();
            effect->setOpacity(s_disabledPaneOpacity);
            // flag this as our effect so we can get rid of it later
            effect->setProperty(s_viewPaneManagerDisableEffectPropertyName, true);

            widget->setGraphicsEffect(effect.release());

            // ensure the widget is redrawn with the updated graphical effect
            ForceWidgetRedraw(widget);
        }
    }

    void SetWidgetInteractEnabled(QWidget* widget, const bool on)
    {
        widget->setEnabled(on);

        if (on)
        {
            DisableViewPaneDisabledGraphicsEffect(widget);
        }
        else
        {
            EnableViewPaneDisabledGraphicsEffect(widget);
        }
    }
} // namespace AzQtComponents
