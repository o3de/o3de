/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AzCore/Utils/Utils.h"

#include <OpenParticleSystemEditorWindow.h>
#include <Editor/EditorParticleSystemComponentRequestBus.h>

#include <AtomToolsFramework/Util/Util.h>
#include <AssetBrowser/AssetBrowserBus.h>
#include <ViewInspector.h>
#include <EmitterInspector.h>
#include <LevelOfDetailInspector.h>
#include <InputController/OpenParticleEditorViewportInputControllerBus.h>
#include <Window/Controls/PropertyDistCtrl.h>
#include <Window/Controls/PropertyGradientColorCtrl.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowNotificationBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>


#include <QMessageBox>
#include <QStatusBar>
#include <QLabel>
#include <QFileDialog>

#define MAX_EMITTER_WIDGET_NUM 10

void QInitResourceOpenParticleEditor() { Q_INIT_RESOURCE(OpenParticleEditor); }
void QCleanupResourceOpenParticleEditor() { Q_CLEANUP_RESOURCE(OpenParticleEditor); }

namespace OpenParticleSystemEditor
{
    OpenParticleSystemEditorWindow::OpenParticleSystemEditorWindow(QWidget* parent)
        : AzQtComponents::DockMainWindow(parent)
        , m_fancyDockingManager(new AzQtComponents::FancyDocking(this, "particlestudiosdk"))
    {
        QInitResourceOpenParticleEditor();
        setObjectName("ParticleEditorWindow");
        m_menuView = menuBar()->addMenu(QCoreApplication::translate("OpenParticleSystemEditorWindow", "&View"));
        SetupCentral();
        SetupDocking();
        SetupMenu();

        m_statusMessage = new QLabel(statusBar());
        statusBar()->addPermanentWidget(m_statusMessage, 1);

        m_dockActions.clear();

        m_document.reset(new ParticleDocument());
        EditorWindowRequestsBus::Handler::BusConnect();
        OpenParticle::EditorParticleOpenParticleRequestsBus::Handler::BusConnect();

        AZ::TickBus::QueueFunction([]()
        {
            EBUS_EVENT(OpenParticleEditorViewportInputControllerRequestBus, Reset);
        });

        SetDistCtrlBusIDName();
        RegisterDistCtrlHandlers();
        SetGradientColorBusIDName();
        RegisterGradientColorPropertyHandler();
    }

    OpenParticleSystemEditorWindow::~OpenParticleSystemEditorWindow()
    {
        EditorWindowRequestsBus::Handler::BusDisconnect();
        OpenParticle::EditorParticleOpenParticleRequestsBus::Handler::BusDisconnect();
        UnregisterDistCtrlHandlers();
        UnregisterGradientColorPropertyHandler();
        QCleanupResourceOpenParticleEditor();
    }

    void OpenParticleSystemEditorWindow::SetupMenu()
    {
        m_menuBar = new QMenuBar(this);
        setMenuBar(m_menuBar);

        m_menuFile = m_menuBar->addMenu(QCoreApplication::translate("OpenParticleSystemEditorWindow", "&File"));

        m_menuFile->addAction(QCoreApplication::translate("OpenParticleSystemEditorWindow", "&New..."),
            [this]()
            {
                AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();

                QString fileName = QFileDialog::getSaveFileName(
                    nullptr,
                    "Create New Particle",
                    projectPath.c_str(),
                    "Particle (*.particle)"
                );

                if (fileName.isEmpty())
                {
                    return;
                }

                QFileInfo fileInfo(fileName);

                AZStd::string fullFilepath;
                AZ::StringFunc::Path::ConstructFull(fileInfo.path().toUtf8(),
                                                    fileInfo.fileName().toUtf8(),
                                                    ".particle",
                                                    fullFilepath);

                OpenParticle::EditorParticleSystemComponentRequestBus::Broadcast(&OpenParticle::EditorParticleSystemComponentRequestBus::Events::CreateNewParticle,
                                                                    fullFilepath);

                OpenDocument(fullFilepath);

            }, QKeySequence::New);

        m_menuFile->addAction(
            QCoreApplication::translate("OpenParticleSystemEditorWindow", "&Open..."),
            [this]()
            {
                AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(azrtti_typeid<OpenParticle::ParticleAsset>());
                AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, parentWidget());
                if (!selection.IsValid())
                {
                    AZ_TracePrintf("SetupMenu", "PickAssets failed\n");
                    return;
                }
                const AZStd::string filePath = selection.GetResult()->GetParent()->GetFullPath();
                if (!filePath.empty())
                {
                    OpenDocument(filePath);
                    SetStatusMessage(tr("Particle opened: %1").arg(filePath.c_str()));
                }
            },
            QKeySequence::Open);

        m_menuFile->addSeparator();

        m_menuFile->addAction(
        QCoreApplication::translate("OpenParticleSystemEditorWindow", "&Save"),
        [this]()
        {
            SaveDocument();
        },
        QKeySequence::Save);


        m_menuFile->addAction(
        QCoreApplication::translate("OpenParticleSystemEditorWindow", "&Save All"),
        [this]()
        {
            SaveAllDocuments();
        },
        QKeySequence::UnknownKey);

        // Add all View DockWidget panes.
        m_menuView = menuBar()->addMenu(QCoreApplication::translate("OpenParticleSystemEditorWindow", "&View"));
        QList<QDockWidget*> list = findChildren<QDockWidget*>();
        for (auto p : list)
        {
            if (p->parent() == this)
            {
                m_menuView->addAction(p->toggleViewAction());
            }
        }
    }

    void OpenParticleSystemEditorWindow::SetupCentral()
    {
        auto widget = this->takeCentralWidget();
        delete widget;
        widget = nullptr;
    }

    void OpenParticleSystemEditorWindow::SetDockWidget(const AZStd::string& name, QWidget* widget)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end() || !widget)
        {
            return;
        }

        auto dockWidget = new AzQtComponents::StyledDockWidget(tr(name.c_str()));
        dockWidget->setObjectName(QString("%1_DockWidget").arg(name.c_str()));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        dockWidget->setMinimumSize(QSize(300, 300));
        widget->setObjectName(tr(name.c_str()));
        widget->setParent(dockWidget);
        dockWidget->setWidget(widget);
        dockWidget->setFloating(false);

        if (dockWidget)
        {
            m_dockWidgets[name] = dockWidget;
        }
        m_dockActions[name] = m_menuView->addAction(
            name.c_str(),
            [this, name]()
            {
                SetDockWidgetVisible(name, !IsDockWidgetVisible(name));
            });
    }

    void OpenParticleSystemEditorWindow::SetupDocking()
    {
        auto viewInspector = new ViewInspector(this);
        const auto Preview = QCoreApplication::translate("OpenParticleSystemEditorWindow", "Preview");
        AddDockWidget(Preview.toUtf8().data(), viewInspector, Qt::LeftDockWidgetArea);

        auto effectorInspector = new EffectorInspector(this);
        m_effectorInspector = effectorInspector;
        const auto Detail = QCoreApplication::translate("OpenParticleSystemEditorWindow", "Detail");
        SetDockWidget(Detail.toUtf8().data(), effectorInspector);

        auto emitterInspector = new EmitterInspector(effectorInspector, this);
        const auto Emitter = QCoreApplication::translate("OpenParticleSystemEditorWindow", "Emitter");
        SetDockWidget(Emitter.toUtf8().data(), emitterInspector);

        auto levelOfDetailInspector = new LevelOfDetailInspector(this);
        const auto LOD = QCoreApplication::translate("OpenParticleSystemEditorWindow", "Level Of Detail");
        SetDockWidget(LOD.toUtf8().data(), levelOfDetailInspector);

        splitDockWidget(m_dockWidgets[Preview.toUtf8().data()], m_dockWidgets[Detail.toUtf8().data()], Qt::Horizontal);
        splitDockWidget(m_dockWidgets[Preview.toUtf8().data()], m_dockWidgets[Emitter.toUtf8().data()], Qt::Horizontal);
        splitDockWidget(m_dockWidgets[Preview.toUtf8().data()], m_dockWidgets[LOD.toUtf8().data()], Qt::Vertical);

        m_dockWidgets[Preview.toUtf8().data()]->setFloating(false);
    }

    bool OpenParticleSystemEditorWindow::SaveDialog()
    {
        QMessageBox::StandardButton button = QMessageBox::No;
        for (const auto& document : m_tabWidgetDocument)
        {
            if (m_opened && document.second->IsModified())
            {
                QString saveParticleFile = tr("Do you want to save changes to %1 ?").arg(QString::fromStdString(document.first.c_str()));
                button = QMessageBox::question(
                    this, tr("Particle file has unsaved changes"), saveParticleFile,
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                if (QMessageBox::Yes == button)
                {
                    document.second->Save();
                }
            }
        }
        return button & (QMessageBox::Yes | QMessageBox::No);
    }

    void OpenParticleSystemEditorWindow::closeEvent(QCloseEvent* event)
    {
        if (!SaveDialog())
        {
            event->ignore();
            return;
        };

        AtomToolsFramework::AtomToolsMainWindowNotificationBus::Broadcast(
            &AtomToolsFramework::AtomToolsMainWindowNotifications::OnMainWindowClosing);
    }

    void OpenParticleSystemEditorWindow::ActivateWindow()
    {
        activateWindow();
        raise();
    }

    void OpenParticleSystemEditorWindow::OpenDocument(const AZStd::string& path)
    {
        AZStd::string emitterInspectorTitle;
        AzFramework::StringFunc::Path::GetFileName(path.c_str(), emitterInspectorTitle);

        if (RaiseOpendEmitter(emitterInspectorTitle.c_str()))
        {
            if (m_opened && m_tabWidgetDocument[emitterInspectorTitle]->IsModified())
            {
                QMessageBox::StandardButton button = QMessageBox::No;
                button = QMessageBox::question(this, tr("Particle file has unsaved changes"),
                    tr("Do you want to save changes to %1 ?")
                        .arg(QString::fromStdString(emitterInspectorTitle.c_str())),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                if (QMessageBox::Yes == button)
                {
                    m_tabWidgetDocument[emitterInspectorTitle]->Save();
                }
            }
            auto document = m_tabWidgetDocument.find(emitterInspectorTitle);
            if (document != m_tabWidgetDocument.end())
            {
                AZStd::string isPastedWidget = "";
                EBUS_EVENT_RESULT(isPastedWidget, ParticleDocumentRequestBus, GetCopyWidgetName);
                if (isPastedWidget == emitterInspectorTitle)
                {
                    // if cloed widget is the widget which "copy" from, set copyName empty
                    EBUS_EVENT(ParticleDocumentRequestBus, SetCopyName, "");
                }
                document->second.reset(new ParticleDocument());
                if (m_emitterTabWidget != nullptr && m_emitterTabWidget->count() <= 0)
                {
                    document->second->UpdateAsset();
                    m_effectorInspector->Init("");
                }
                m_tabWidgetDocument.erase(document);
            }
            RemoveDockWidget(emitterInspectorTitle);
        }

        if (m_dockWidgets.size() > MAX_EMITTER_WIDGET_NUM)
        {
            AZ_Printf("Emitter", "max Emitter widget");
            return;
        }

        if (!m_opened && m_document != nullptr)
        {
            m_opened = m_document->Open(path.c_str());
            m_tabWidgetDocument[emitterInspectorTitle.c_str()] = AZStd::move(m_document);
        }
        SetTabWidget(path);
        SetStatusMessage(tr("Particle opened: %1").arg(path.c_str()));
        activateWindow();
    }

    void OpenParticleSystemEditorWindow::SetTabWidget(const AZStd::string& path)
    {
        AZStd::string emitterInspectorTitle;
        AzFramework::StringFunc::Path::GetFileName(path.c_str(), emitterInspectorTitle);
        auto emitterInspector = new EmitterInspector(m_effectorInspector, this, emitterInspectorTitle.c_str());

        QString newEmitter = QCoreApplication::translate("OpenParticleSystemEditorWindow", emitterInspectorTitle.c_str());

        SetEmitterDockWidget(newEmitter.toUtf8().data(), emitterInspector, Qt::LeftDockWidgetArea);
        Checked(newEmitter);

        for (auto emitterDockWidgetPair : m_dockWidgets)
        {
            if (emitterDockWidgetPair.second && emitterDockWidgetPair.second->findChild<EmitterInspector*>())
            {
                if (m_dockWidgets.find(newEmitter.toUtf8().data()) != m_dockWidgets.end() && 
                        emitterDockWidgetPair.second == m_dockWidgets[newEmitter.toUtf8().data()])
                {
                    continue;
                }
                else
                {
                    m_emitterTabWidget = m_fancyDockingManager->tabifyDockWidget(emitterDockWidgetPair.second, m_dockWidgets[newEmitter.toUtf8().data()], this);
                    break;
                }
            }
        }

        SetDistCtrlBusIDName(emitterInspectorTitle);
        SetGradientColorBusIDName(emitterInspectorTitle);

        AZStd::string pastedName = "";
        EBUS_EVENT_RESULT(pastedName, ParticleDocumentRequestBus, GetCopyWidgetName);
        m_tabWidgetDocument[emitterInspectorTitle].reset(new ParticleDocument());
        m_tabWidgetDocument[emitterInspectorTitle]->Open(path);
        EBUS_EVENT(ParticleDocumentRequestBus, SetCopyName, pastedName);

        AzQtComponents::StyledDockWidget* styleDockWidget = qobject_cast<AzQtComponents::StyledDockWidget*>(m_dockWidgets[newEmitter.toUtf8().data()]);
        connect(styleDockWidget, &AzQtComponents::StyledDockWidget::aboutToClose, [this, emitterInspectorTitle]() {
            if (m_opened && m_tabWidgetDocument[emitterInspectorTitle]->IsModified())
            {
                QMessageBox::StandardButton button = QMessageBox::No;
                button = QMessageBox::question(
                    this, tr("Particle file has unsaved changes"), tr("Do you want to save changes to %1 ?").arg(QString::fromStdString(emitterInspectorTitle.c_str())),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                if (QMessageBox::Yes == button)
                {
                    m_tabWidgetDocument[emitterInspectorTitle]->Save();
                }
            }
            auto document = m_tabWidgetDocument.find(emitterInspectorTitle);
            if (document != m_tabWidgetDocument.end())
            {
                EBUS_EVENT(ParticleDocumentRequestBus, SetCopyName, "");
                document->second.reset();
                m_tabWidgetDocument.erase(document);
            }
            RemoveDockWidget(emitterInspectorTitle.c_str());
            if (m_tabWidgetDocument.empty())
            {
                // Show default emitter tab when no document opened to keep layout
                m_emitterTabWidget->setTabVisible(0, true);
                // Clear detail dock
                m_effectorInspector->Init("");
            }
        });

         connect(m_dockWidgets[newEmitter.toUtf8().data()], &QDockWidget::visibilityChanged, [this, emitterInspectorTitle](bool isVisible) {
             if (isVisible) {
                 SetDistCtrlBusIDName(emitterInspectorTitle);
                 SetGradientColorBusIDName(emitterInspectorTitle);
                 m_effectorInspector->Init(emitterInspectorTitle);
                 m_tabWidgetDocument[emitterInspectorTitle]->UpdateAsset();
             } else {
                 ParticleDocumentNotifyBus::Broadcast(&ParticleDocumentNotifyBus::Handler::OnDocumentInvisible);
             }
         });

        connect(m_emitterTabWidget, &QTabWidget::currentChanged, [this](int index) {
            if (index >= 0 && m_emitterTabWidget->tabText(index).isEmpty())
            {
                return;
            }

            const QString name = m_emitterTabWidget->tabText(index);
            m_currentTabName = name.toUtf8().data();
        });

        // Hide default emitter tab when any document opened
        m_emitterTabWidget->setTabVisible(0, false);
    }

    void OpenParticleSystemEditorWindow::SetEmitterDockWidget(const AZStd::string& name, QWidget* widget, AZ::u32 area)
    {
        AzQtComponents::StyledDockWidget* dockWidget = new AzQtComponents::StyledDockWidget(tr(name.c_str()));
        dockWidget->setObjectName(QString("%1_DockWidget").arg(name.c_str()));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);

        widget->setObjectName(tr(name.c_str()));
        widget->setParent(dockWidget);
        dockWidget->setWidget(widget);
        dockWidget->setFloating(false);
        addDockWidget(aznumeric_cast<Qt::DockWidgetArea>(area), dockWidget);

        if (dockWidget)
        {
            m_dockWidgets[name] = dockWidget;
        }

        m_dockActions[name] = m_menuView->addAction(name.c_str(), [this, name]() {
            if (m_dockActions[name]->isChecked()) {
                m_emitterTabWidget->addTab(m_dockWidgets[name]);
            } else {
                m_emitterTabWidget->removeTab(m_dockWidgets[name]);
            }
        });
        m_dockActions[name]->setCheckable(true);
    }

    bool OpenParticleSystemEditorWindow::RaiseOpendEmitter(const QString& path)
    {
        for (auto emitterDockWidgetPair : m_dockWidgets)
        {
            if (emitterDockWidgetPair.second->widget()->windowTitle() == path)
            {
                m_emitterTabWidget->setCurrentWidget(emitterDockWidgetPair.second);
                m_tabWidgetDocument[path.toStdString().c_str()]->UpdateAsset();
                return true;
            }
        }
        return false;
    }

    void OpenParticleSystemEditorWindow::CreateParticleFile(const AZStd::string& path)
    {
        AZStd::string emitterInspectorTitle;
        AzFramework::StringFunc::Path::GetFileName(path.c_str(), emitterInspectorTitle);

        if (!m_opened && m_document != nullptr)
        {
            m_opened = m_document->CreateParticle(path.c_str());
            m_tabWidgetDocument[emitterInspectorTitle] = AZStd::move(m_document);
        }
        else
        {
            m_tabWidgetDocument[emitterInspectorTitle].reset(new ParticleDocument());
            m_tabWidgetDocument[emitterInspectorTitle]->CreateParticle(path);
        }

        SetStatusMessage(QString("Particle created: %1").arg(path.c_str()));
    }

    void OpenParticleSystemEditorWindow::SaveDocument()
    {
        if (m_emitterTabWidget && m_emitterTabWidget->count() > 0)
        {
            const int currentIndex = m_emitterTabWidget->currentIndex();
            if (currentIndex >= 0)
            {
                const QString currentTabName = m_emitterTabWidget->tabText(currentIndex);
                if (!currentTabName.isEmpty())
                {
                    const auto document = m_tabWidgetDocument.find(currentTabName.toUtf8().data());
                    if (document != m_tabWidgetDocument.end() &&
                        m_opened &&
                        document->second->IsModified())
                    {
                        document->second->Save();

                        SetStatusMessage(QString(tr("Particle saved: %1")).arg(currentTabName));
                    }
                }
            }
        }
    }

    void OpenParticleSystemEditorWindow::SaveAllDocuments()
    {
        for (const auto& document : m_tabWidgetDocument)
        {
            if (m_opened && document.second->IsModified())
            {
                document.second->Save();

                SetStatusMessage(QString(tr("Particle saved: %1")).arg(document.first.c_str()));
            }
        }
    }

    void OpenParticleSystemEditorWindow::SetStatusMessage(const QString& message)
    {
        m_statusMessage->setText(QString("<font color=\"White\">%1</font>").arg(message));
    }

    void OpenParticleSystemEditorWindow::SetDockWidgetVisible(const AZStd::string& name, bool visible)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            dockWidgetItr->second->setVisible(visible);
        }
    }

    bool OpenParticleSystemEditorWindow::IsDockWidgetVisible(const AZStd::string& name) const
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            return dockWidgetItr->second->isVisible();
        }
        return false;
    }

    bool OpenParticleSystemEditorWindow::AddDockWidget(const AZStd::string& name, QWidget* widget, AZ::u32 area)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end() || !widget)
        {
            return false;
        }

        auto dockWidget = new AzQtComponents::StyledDockWidget(name.c_str());
        dockWidget->setObjectName(QString("%1_DockWidget").arg(name.c_str()));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        widget->setObjectName(name.c_str());
        widget->setParent(dockWidget);
        dockWidget->setWidget(widget);
        addDockWidget(aznumeric_cast<Qt::DockWidgetArea>(area), dockWidget);

        if (dockWidget)
        {
            m_dockWidgets[name] = dockWidget;
        }

        m_dockActions[name] = m_menuView->addAction(
            name.c_str(),
            [this, name]()
            {
                SetDockWidgetVisible(name, !IsDockWidgetVisible(name));
            });
        m_dockActions[name]->setCheckable(true);
        return true;
    }

    void OpenParticleSystemEditorWindow::Checked(const QString& name)
    {
        auto dockActionItr = m_dockActions.find(AZStd::string(name.toUtf8().data()));
        if (dockActionItr != m_dockActions.end())
        {
            dockActionItr->second->setChecked(true);
        }
    }

    void OpenParticleSystemEditorWindow::UnChecked(const QString& name)
    {
        auto dockActionItr = m_dockActions.find(AZStd::string(name.toUtf8().data()));
        if (dockActionItr != m_dockActions.end())
        {
            dockActionItr->second->setChecked(false);
        }
    }

    void OpenParticleSystemEditorWindow::RemoveDockWidget(const AZStd::string& name)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            dockWidgetItr->second->deleteLater();
            m_dockWidgets.erase(dockWidgetItr);
        }
        auto dockActionItr = m_dockActions.find(name);
        if (dockActionItr != m_dockActions.end())
        {
            delete dockActionItr->second;
            m_dockActions.erase(dockActionItr);
        }
    }

    AZStd::vector<AZStd::string> OpenParticleSystemEditorWindow::GetDockWidgetNames() const
    {
        AZStd::vector<AZStd::string> names;
        names.reserve(m_dockWidgets.size());
        for (const auto& dockWidgetPair : m_dockWidgets)
        {
            names.push_back(dockWidgetPair.first);
        }
        return names;
    }

    void OpenParticleSystemEditorWindow::OpenParticleFile(const AZStd::string& sourcePath)
    {
        OpenDocument(sourcePath);
    }
} // namespace OpenParticleSystemEditor
