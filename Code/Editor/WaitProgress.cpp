/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "WaitProgress.h"

#include "MainWindow.h"         // for MainWindow
#include "CryEdit.h"            // for EditorIdleProcessingBus
#include "MainStatusBar.h"      // for MainStatusBar

#include <QProgressBar>

bool CWaitProgress::m_bInProgressNow = false;

CWaitProgress::CWaitProgress(const QString& lpszText, bool bStart)
    : m_strText(lpszText)
    , m_bStarted(false)
    , m_progressBar(nullptr)
{
    m_bIgnore = false;

    if (bStart)
    {
        Start();
    }
}

CWaitProgress::~CWaitProgress()
{
    if (m_bStarted)
    {
        Stop();
    }
}

void CWaitProgress::Start()
{
    if (m_bStarted)
    {
        Stop();
    }

    if (m_bInProgressNow)
    {
        // Do not affect previously started progress bar.
        m_bIgnore = true;
        m_bStarted = false;
        return;
    }

    // display text in the status bar
    GetIEditor()->SetStatusText(m_strText);

    // switch on wait cursor
    qApp->setOverrideCursor(Qt::BusyCursor);

    m_bStarted = true;
    m_percent = 0;

    // because Idle Processing was constantly adding new events that made the event loop in Step() spin forever...
    EditorIdleProcessingBus::Broadcast(&EditorIdleProcessing::DisableIdleProcessing); 
}

void CWaitProgress::Stop()
{
    if (!m_bStarted)
    {
        return;
    }

    delete m_progressBar;
    m_progressBar = nullptr;

    // switch off wait cursor
    qApp->restoreOverrideCursor();
    m_bInProgressNow = false;
    m_bStarted = false;

    EditorIdleProcessingBus::Broadcast(&EditorIdleProcessing::EnableIdleProcessing);
}

bool CWaitProgress::Step(int nPercentage)
{
    if (m_bIgnore)
    {
        return true;
    }

    if (!m_bStarted)
    {
        Start();
    }

    if (m_percent == nPercentage)
    {
        return true;
    }

    m_percent = nPercentage;

    if (nPercentage >= 0)
    {
        if (nPercentage > 100)
        {
            nPercentage = 100;
        }

        // create or update a progress control in the status bar
        if (!m_progressBar)
        {
            CreateProgressControl();
        }

        if (m_progressBar)
        {
            m_progressBar->setValue(nPercentage);
        }
    }

    // Use the oportunity to process windows messages here.
    static const AZ::u64 TIMEOUT_MS = 1;
    qApp->processEvents(QEventLoop::AllEvents, TIMEOUT_MS);

    return true;
}

void CWaitProgress::SetText(const QString& lpszText)
{
    if (m_bIgnore)
    {
        return;
    }

    m_strText = lpszText;
    GetIEditor()->SetStatusText(m_strText);
}

void CWaitProgress::CreateProgressControl()
{
    assert(m_progressBar == nullptr);

    MainStatusBar* statusBar = MainWindow::instance()->StatusBar();
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    statusBar->insertWidget(1, m_progressBar, 250);

    m_bInProgressNow = true;
}
