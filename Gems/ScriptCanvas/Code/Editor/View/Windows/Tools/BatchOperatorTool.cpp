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
#include "precompiled.h"

#include <QCoreApplication>

#include <ISystem.h>
#include <IConsole.h>

#include <Editor/View/Windows/Tools/BatchOperatorTool.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <Editor/View/Widgets/GraphTabBar.h>
#include <Editor/View/Windows/MainWindow.h>

namespace ScriptCanvasEditor
{
    //////////////////////
    // BatchOperatorTool
    //////////////////////
    
    BatchOperatorTool::BatchOperatorTool(MainWindow* mainWindow, QStringList directories, QString progressDialogTitle)
        : m_mainWindow(mainWindow)
        , m_originalActive(-1)
        , m_originalActiveTab(-1)        
        , m_progressDialog(nullptr)
    {
        ISystem* system = nullptr;
        CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

        ICVar* cvar = system->GetIConsole()->GetCVar("ed_KeepEditorActive");

        if (cvar)
        {
            m_originalActive = cvar->GetIVal();
            cvar->Set(1);
        }

        m_originalActiveTab = m_mainWindow->m_tabBar->currentIndex();

        m_progressDialog = new QProgressDialog(m_mainWindow);

        m_progressDialog->setWindowFlags(m_progressDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
        m_progressDialog->setLabelText(progressDialogTitle);
        m_progressDialog->setWindowModality(Qt::WindowModal);
        m_progressDialog->setMinimum(0);
        m_progressDialog->setMaximum(0);
        m_progressDialog->setMinimumDuration(0);
        m_progressDialog->setAutoClose(false);
        m_progressDialog->setCancelButton(nullptr);

        m_progressDialog->show();

        m_connection = QObject::connect(m_progressDialog, &QProgressDialog::canceled, [this]()
        {
                CancelOperation();
        });

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        for (QString directory : directories)
        {
            QDirIterator* directoryIterator = new QDirIterator(directory, QDirIterator::Subdirectories);

            m_directoryIterators.push_back(directoryIterator);
        }

        QTimer::singleShot(0, [this]()
        {
            TickIterator();
        });
    }
    
    BatchOperatorTool::~BatchOperatorTool()
    {
        ISystem* system = nullptr;
        CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

        ICVar* cvar = system->GetIConsole()->GetCVar("ed_KeepEditorActive");

        if (cvar)
        {
            cvar->Set(m_originalActive);
        }

        delete m_progressDialog;

        for (QDirIterator* dirIterator : m_directoryIterators)
        {
            delete dirIterator;
        }

        QObject::disconnect(m_connection);
    }
    
    void BatchOperatorTool::SignalOperationComplete()
    {
        if (m_cancelled)
        {
            OnOperationCancelled();

            m_directoryIterators.clear();
            m_originalActiveTab = -1;

            OnBatchComplete();
        }
        else
        {
            TickIterator();
        }
    }

    MainWindow* BatchOperatorTool::GetMainWindow() const
    {
        return m_mainWindow;
    }

    QProgressDialog* BatchOperatorTool::GetProgressDialog() const
    {
        return m_progressDialog;
    }

    void BatchOperatorTool::OnOperationCancelled()
    {

    }

    void BatchOperatorTool::CancelOperation()
    {        
        m_cancelled = true;
    }
    
    void BatchOperatorTool::IterateOverDirectory(QDir directory)
    {
        QDirIterator* directoryIterator = new QDirIterator(directory.absolutePath(), QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);        

        m_directoryIterators.emplace_back(directoryIterator);            
    }

    void BatchOperatorTool::TickIterator()
    {
        bool tailRecurse = false;

        do
        {
            tailRecurse = false;
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

            if (!m_directoryIterators.empty())
            {
                QDirIterator* dirIterator = m_directoryIterators.back();

                if (dirIterator->hasNext())
                {
                    QString newElement = dirIterator->next();

                    m_progressDialog->setLabelText(QString("Scanning %1...\n").arg(newElement));

                    if (newElement.endsWith("."))
                    {
                        tailRecurse = true;
                    }
                    else if (newElement.endsWith(".scriptcanvas"))
                    {
                        OperationStatus status = OperateOnFile(newElement);
                        tailRecurse = (status == OperationStatus::Complete);
                    }
                    else
                    {
                        QFileInfo fileInfo(newElement);

                        if (fileInfo.isDir())
                        {
                            IterateOverDirectory(QDir(newElement));
                            tailRecurse = true;
                        }
                        else
                        {
                            tailRecurse = true;
                        }
                    }
                }
                else
                {
                    delete dirIterator;
                    m_directoryIterators.pop_back();

                    tailRecurse = true;
                }
            }
            else
            {
                OnBatchComplete();
            }
        } while (tailRecurse);
    }

    void BatchOperatorTool::OnBatchComplete()
    {
        if (m_progressDialog)
        {
            m_progressDialog->hide();
            delete m_progressDialog;
            m_progressDialog = nullptr;
        }

        if (m_originalActiveTab >= 0)
        {
            m_mainWindow->m_tabBar->setCurrentIndex(m_originalActiveTab);
        }

        // Essentially a delete this. From this point on this element is garbage.
        m_mainWindow->SignalBatchOperationComplete(this);
    }
}
