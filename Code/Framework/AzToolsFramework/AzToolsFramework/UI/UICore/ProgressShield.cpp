/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/chrono.h>

#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/UI/UICore/ui_ProgressShield.h>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace ProgressShieldInternal
    {
        void PumpEvents()
        {
            // re: 16 msec - pump QueuedEvents at a min of ~60 FPS
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents, 16);
            AZ::TickBus::ExecuteQueuedEvents();
        }
    } // namespace ProgressShieldInternal

    ProgressShield::ProgressShield(QWidget* pParent)
        : QWidget(pParent, Qt::WindowFlags(Qt::ToolTip | Qt::BypassWindowManagerHint | Qt::FramelessWindowHint))
    {
        AZ_Assert(pParent && parentWidget(), "There must be a parent.");
        if (pParent && parentWidget())
        {
            uiConstructor = azcreate(Ui::progressShield, ());
            uiConstructor->setupUi(this);
            setAttribute(Qt::WA_TranslucentBackground);
            setAttribute(Qt::WA_TransparentForMouseEvents);
            setAttribute(Qt::WA_NoSystemBackground);
            setGeometry(parentWidget()->geometry()); // because we're a tooltip, we work in absolute coordinates.
            pParent->installEventFilter(this);
            setProgress(0, 0, tr("Loading..."));
            UpdateGeometry();
        }
    }

    ProgressShield::~ProgressShield()
    {
        parentWidget()->removeEventFilter(this);
        azdestroy(uiConstructor);
    }

    void ProgressShield::LegacyShowAndWait(QWidget* parent, QString label, AZStd::function<bool(int& current, int& max)> completeCallback, int delayMS)
    {
        using AzSysClock = AZStd::chrono::steady_clock;
        using AzMillisec = AZStd::chrono::milliseconds;
        bool showShield = false;

        // delay showing the progress shield when we have callbacks that complete quickly
        int current = 0, max = 0;
        AzSysClock::time_point start = AzSysClock::now();
        while (!completeCallback(current, max))
        {
            ProgressShieldInternal::PumpEvents();
            if (AZStd::chrono::duration_cast<AzMillisec>(AzSysClock::now() - start) > AzMillisec(delayMS))
            {
                showShield = true;
                break;
            }
        }

        // we've exceeded the delay, show the progress shield and wait for callback to complete
        if (showShield)
        {
            ProgressShield shield(parent);
            current = max = 0;

            shield.show();
            while (!completeCallback(current, max))
            {
                shield.setProgress(current, max, label);
                ProgressShieldInternal::PumpEvents();
            }
        }
    }

    void ProgressShield::UpdateGeometry()
    {
        setGeometry(parentWidget()->geometry()); // because we're a tooltip window (not a child), we work in absolute coordinates.
    }


    bool ProgressShield::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::Resize)
        {
            UpdateGeometry();
        }

        // hand it to the owner
        return QObject::eventFilter(obj, event);
    }

    void ProgressShield::setProgress(int current, int max, const QString& label)
    {
        uiConstructor->progressLabel->setText(label);
        uiConstructor->progressLabel->show();
        uiConstructor->progressBar->setMaximum(max);
        uiConstructor->progressBar->setValue(current);
    }

} // namespace AzToolsFramework

#include "UI/UICore/moc_ProgressShield.cpp"
