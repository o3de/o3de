/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzToolsFramework/UI/Docking/DockWidgetUtils.h>

AZ_PUSH_DISABLE_WARNING(4251 4244 4458, "-Wunknown-warning-option") // 4251: 'QTextStream::d_ptr': class 'QScopedPointer<QTextStreamPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QTextStream'
                                                                    // 4244: '=': conversion from 'int' to 'qint8', possible loss of data
                                                                    // 4458: declaration of 'parent' hides class member
#include <QTimer>
#include <QTabBar>
#include <QMainWindow>
#include <QDockWidget>
#include <QDebug>
#include <QDataStream>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    bool DockWidgetUtils::containsDockWidget(QObject *o)
    {
        if (!o)
        {
            return false;
        }

        if (qobject_cast<QDockWidget*>(o))
        {
            return true;
        }

        const auto children = o->children();
        for (auto child : children)
        {
            if (containsDockWidget(child))
            {
                return true;
            }
        }

        return false;
    }

    QList<QWidget*> DockWidgetUtils::getDockWindowGroups(QMainWindow *mainWindow)
    {
        const QObjectList children = mainWindow->children();
        QList<QWidget*> result;
        for (auto child : children)
        {
            if (auto w = qobject_cast<QWidget*>(child))
            {
                if (QString(w->metaObject()->className()) == QStringLiteral("QDockWidgetGroupWindow"))
                {
                    result.append(w);
                }
            }
        }

        return result;
    }

    void DockWidgetUtils::deleteWindowGroups(QMainWindow *mainWindow, bool onlyGhosts)
    {
        const QList<QWidget*> dockWindowGroups = getDockWindowGroups(mainWindow);
        for (auto dwgw : dockWindowGroups)
        {
            const bool isGhost = !containsDockWidget(dwgw);
            if (!onlyGhosts || isGhost)
            {
                for (auto c2 : dwgw->children()) {
                    if (auto dock = qobject_cast<QDockWidget*>(c2))
                    {
                        //qDebug() << "Reparenting one " << dock->windowTitle();
                        dock->setParent(mainWindow);
                    }
                    else if (auto tb = qobject_cast<QTabBar*>(c2))
                    {
                        //qDebug() << "Reparenting a tab bar. Visible= " << tb->isVisible();
                        tb->setParent(mainWindow);
                    }
                }
                //qDebug() << "Deleting dwgw";
                delete dwgw;
            }
        }
    }

    void DockWidgetUtils::dumpDockWidgets(QMainWindow *mainWindow)
    {
        Q_ASSERT(mainWindow);
        qDebug() << "dumpDockWidgets START";
        const QList<QWidget*> dockWindowGroups = DockWidgetUtils::getDockWindowGroups(mainWindow);
        for (auto dwgw : dockWindowGroups)
        {
            qDebug() << "    Got one QDockWidgetGroupWindow. visible="
                << dwgw->isVisible()
                << "; enabled =" << dwgw->isEnabled()
                << (!containsDockWidget(dwgw) ? "; ghost" : "");
            for (auto c : dwgw->children()) {
                if (auto w = qobject_cast<QWidget*>(c))
                {
                    qDebug() << "        * " << w
                        << "visible=" << w->isVisible()
                        << "enabled=" << w->isEnabled();
                }
                if (auto dock = qobject_cast<QDockWidget*>(c))
                {
                    qDebug() << "        "
                        << "geometry=" << dock->geometry()
                        << "title=" << dock->windowTitle()
                        << "isFloating=" << dock->isFloating()
                        << "area=" << mainWindow->dockWidgetArea(dock);
                }
            }
        }

        for (auto c : mainWindow->children())
        {
            if (auto dock = qobject_cast<QDockWidget*>(c))
            {
                qDebug() << "    Got one QDockWidget. Visible="
                    << dock->isVisible()
                    << "geometry=" << dock->geometry()
                    << "title=" << dock->windowTitle()
                    << "isFloating=" << dock->isFloating()
                    << "enabled=" << dock->isEnabled()
                    << "area=" << mainWindow->dockWidgetArea(dock);
            }
        }
        qDebug() << "dumpDockWidgets END";
    }

    bool DockWidgetUtils::processSavedState(const QByteArray &data, QStringList &)
    {
        if (data.isEmpty())
        {
            return false;
        }

        // #QT6_TODO
        qDebug() << "DockWidgetUtils::processSavedState END";
        return true;
    }

    bool DockWidgetUtils::isDockWidgetWindowGroup(QWidget* w)
    {
        return w && QString(w->metaObject()->className()) == QStringLiteral("QDockWidgetGroupWindow");
    }

    bool DockWidgetUtils::isInDockWidgetWindowGroup(QDockWidget* w)
    {
        return w && isDockWidgetWindowGroup(w->parentWidget());
    }

    void DockWidgetUtils::correctVisibility(QDockWidget* dw)
    {
        if (isInDockWidgetWindowGroup(dw) && !dw->parentWidget()->isVisible())
        {
            dw->parentWidget()->show();
        }
    }

    void DockWidgetUtils::startPeriodicDebugDump(QMainWindow *mainWindow)
    {
        const auto t = new QTimer{ mainWindow };
        t->start(5000);
        QObject::connect(t, &QTimer::timeout, mainWindow, [mainWindow]
        {
            DockWidgetUtils::dumpDockWidgets(mainWindow);
        });
    }

    bool DockWidgetUtils::hasInvalidDockWidgets(QMainWindow *mainWindow)
    {
        for (auto c : mainWindow->children())
        {
            if (auto dock = qobject_cast<QDockWidget*>(c))
            {
                if (mainWindow->dockWidgetArea(dock) == Qt::NoDockWidgetArea && !dock->isFloating())
                {
                    return true;
                }
            }
        }
        return false;
    }
}
