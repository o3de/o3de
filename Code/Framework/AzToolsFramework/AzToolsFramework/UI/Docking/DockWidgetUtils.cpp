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
#include <QtWidgets/private/qdockarealayout_p.h>
#include <QtWidgets/private/qtoolbararealayout_p.h>
#include <QtWidgets/private/qmainwindowlayout_p.h>
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

    static bool processQDockAreaLayoutInfo(QDataStream &stream, QStringList &dockNames)
    {
        uchar marker;
        stream >> marker;
        if (marker != QDockAreaLayoutInfo::TabMarker && marker != QDockAreaLayoutInfo::SequenceMarker)
        {
            return false;
        }

        const bool tabbed = marker == QDockAreaLayoutInfo::TabMarker;

        int index = -1;
        if (tabbed)
        {
            stream >> index;
        }

        uchar orientation;
        stream >> orientation;
        int cnt;
        stream >> cnt;
        for (int i = 0; i < cnt; ++i)
        {
            uchar nextMarker;
            stream >> nextMarker;
            if (nextMarker == QDockAreaLayoutInfo::WidgetMarker)
            {
                QString name;
                uchar flags;
                stream >> name >> flags;
                qDebug() << "    DockWidgetUtils::processSavedState WidgetMarker name="
                    << name << "; floating=" << !!(flags & 2) << "; visible=" << !!(flags & 1);
                dockNames << name;
                int dummy;
                stream >> dummy >> dummy >> dummy >> dummy;
            }
            else if (nextMarker == QDockAreaLayoutInfo::SequenceMarker)
            {
                qDebug() << "DockWidgetUtils::processSavedState SequenceMarker";
                int dummy;
                stream >> dummy >> dummy >> dummy >> dummy;
                if (!processQDockAreaLayoutInfo(stream, dockNames))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool DockWidgetUtils::processSavedState(const QByteArray &data, QStringList &dockNames)
    {
        if (data.isEmpty())
        {
            return false;
        }

        qDebug() << "DockWidgetUtils::processSavedState";
        QByteArray sd = data;
        QDataStream stream(&sd, QIODevice::ReadOnly);
        int m, v;

        stream >> m >> v;

        if (stream.status() != QDataStream::Ok || m != QMainWindowLayout::VersionMarker || v != 0)
        {
            return false;
        }

        while (!stream.atEnd())
        {
            uchar marker;
            stream >> marker;

            switch (marker)
            {
            case QDockAreaLayout::DockWidgetStateMarker:
            {
                qDebug() << "DockWidgetUtils::processSavedState DockWidgetStateMarker";
                int cnt;
                stream >> cnt;
                for (int i = 0; i < cnt; ++i) {
                    int pos;
                    stream >> pos;
                    QSize size;
                    stream >> size;

                    if (!processQDockAreaLayoutInfo(stream, dockNames))
                    {
                        return false;
                    }
                }

                QSize size;
                stream >> size;
                bool ok = stream.status() == QDataStream::Ok;

                if (ok)
                {
                    int cornerData[4];
                    for (int i = 0; i < 4; ++i)
                    {
                        stream >> cornerData[i];
                    }
                }
                else
                {
                    return false;
                }
                break;
            }
            case QDockAreaLayout::FloatingDockWidgetTabMarker:
            {
                qDebug() << "DockWidgetUtils::processSavedState FloatingDockWidgetTabMarker";
                QRect geometry;
                stream >> geometry;
                if (!processQDockAreaLayoutInfo(stream, dockNames))
                {
                    return false;
                }
                break;
            }
            case QToolBarAreaLayout::ToolBarStateMarker:
            case QToolBarAreaLayout::ToolBarStateMarkerEx:
            {
                qDebug() << "DockWidgetUtils::processSavedState ToolbarMarker";
                int dummyInt;
                int lines;
                stream >> lines;
                for (int j = 0; j < lines; ++j)
                {
                    int pos;
                    stream >> pos;

                    if (pos < 0 || pos >= QInternal::DockCount)
                    {
                        return false;
                    }
                    int cnt;
                    stream >> cnt;
                    for (int k = 0; k < cnt; ++k)
                    {
                        QString dummyString;
                        uchar dummyUChar;
                        stream >> dummyString >> dummyUChar >> dummyInt >> dummyInt >> dummyInt;
                        if (marker == QToolBarAreaLayout::ToolBarStateMarkerEx)
                        {
                            stream >> dummyInt;
                        }
                    }
                }
                break;
            }
            default:
                qDebug() << "Error" << marker;
                return false;
            }
        }

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
