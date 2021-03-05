/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "AzToolsFramework_precompiled.h"
#include "UIFrameworkAPI.h"
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/delegate/delegate.h>

#ifdef Q_OS_WIN
# include <QtGui/qpa/qplatformnativeinterface.h>
#endif

#include <AzToolsFramework/UI/UICore/OverwritePromptDialog.hxx>

#include <QWindow>

namespace AzToolsFramework
{
    namespace
    {
        // an aggregator utility which essentially provides the operation of returning the last result of an ebus event
        // which returned something which returns a non-false value (like for example if its a pointer, then the last non-null value)
        template<class T>
        struct EBusLastNonNullResult
        {
            T value;
            EBusLastNonNullResult() { value = NULL; }
            AZ_FORCE_INLINE void operator=(const T& rhs)
            {
                if (rhs)
                {
                    value = rhs;
                }
            }
            AZ_FORCE_INLINE T& operator->() { return value; }
        };

        template<class T>
        struct EBusAnyTrueResult
        {
            T value;
            AZ_FORCE_INLINE void operator=(const T& rhs) { value = rhs || value; }
            AZ_FORCE_INLINE T& operator->() { return value; }
        };
    }

    static QWindow* windowForWidget(const QWidget* widget)
    {
        QWindow* window = widget->windowHandle();
        if (window)
        {
            return window;
        }
        const QWidget* nativeParent = widget->nativeParentWidget();
        if (nativeParent)
        {
            return nativeParent->windowHandle();
        }
        return 0;
    }

    HWND getHWNDForWidget(const QWidget* widget)
    {
        QWindow* window = windowForWidget(widget);
        if (window && window->handle())
        {
            QPlatformNativeInterface* itf = QGuiApplication::platformNativeInterface();
            return static_cast<HWND>(itf->nativeResourceForWindow(QByteArrayLiteral("handle"), window));
        }
        return 0;
    }

    bool GetOverwritePromptResult(QWidget* pParentWidget, const char* assetNameToOvewrite)
    {
        OverwritePromptDialog dlg(pParentWidget);
        if (assetNameToOvewrite)
        {
            dlg.UpdateLabel(QString::fromUtf8(assetNameToOvewrite));
        }

        if (!dlg.exec() == QDialog::Accepted)
        {
            return false;
        }

        return dlg.m_result;
    }
}