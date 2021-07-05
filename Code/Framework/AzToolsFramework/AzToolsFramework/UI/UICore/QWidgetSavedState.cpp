/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"
#include "QWidgetSavedState.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <QtCore/QByteArray>
#include <QtWidgets/QWidget>

namespace AzToolsFramework
{
    void QWidgetSavedState::CaptureGeometry(QWidget* pWidget)
    {
        AZ_Assert(pWidget, "Null widget passed in to QWidgetSavedState::CaptureGeometry");
        QByteArray savedGeometry = pWidget->saveGeometry();
        m_windowGeometry.assign((AZ::u8*)savedGeometry.begin(), (AZ::u8*)savedGeometry.end());
    }
    void QWidgetSavedState::RestoreGeometry(QWidget* pWidget)
    {
        AZ_Assert(pWidget, "Null widget passed in to QWidgetSavedState::CaptureGeometry");
        if (m_windowGeometry.empty())
        {
            return;
        }
        QByteArray geomData((const char*)m_windowGeometry.data(), (int)m_windowGeometry.size());
        pWidget->restoreGeometry(geomData);
    }

    void QWidgetSavedState::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<QWidgetSavedState>()
                ->Version(1)
                ->Field("m_windowGeometry", &QWidgetSavedState::m_windowGeometry);
        }
    }
}
