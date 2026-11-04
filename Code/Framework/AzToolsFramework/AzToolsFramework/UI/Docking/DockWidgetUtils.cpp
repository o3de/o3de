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

    // Mimic enum found in qtbase\src\widgets\widgets\qdockarealayout_p.h
    enum DockAreaLayoutInfo
    { // sentinel values used to validate state data
        SequenceMarker = 0xfc,
        TabMarker = 0xfa,
        WidgetMarker = 0xfb
    };

    static bool processQDockAreaLayoutInfo(QDataStream &stream, QStringList &dockNames)
    {
        uchar marker;
        stream >> marker;
        if (marker != DockAreaLayoutInfo::TabMarker && marker != DockAreaLayoutInfo::SequenceMarker)
        {
            return false;
        }

        const bool tabbed = marker == DockAreaLayoutInfo::TabMarker;

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
            if (nextMarker == DockAreaLayoutInfo::WidgetMarker)
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
            else if (nextMarker == DockAreaLayoutInfo::SequenceMarker)
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

    // Mimic enum in qtbase\src\widgets\widgets\qmainwindowlayout_p.h
    enum VersionMarkers
    { // sentinel values used to validate state data
        VersionMarker = 0xff
    };

    // Mimic enum in qtbase\src\widgets\widgets\qdockarealayout_p.h
    enum DockAreaLayout
    {
        DockWidgetStateMarker = 0xfd,
        FloatingDockWidgetTabMarker = 0xf9
    };

    // Mimic enum in qtbase\src\widgets\widgets\qtoolbararealayout_p.h
    enum ToolBarAreaLayout
    { // sentinel values used to validate state data
        ToolBarStateMarker = 0xfe,
        ToolBarStateMarkerEx = 0xfc
    };

    bool DockWidgetUtils::processSavedState(const QByteArray &data, QStringList &dockNames)
    {
        if (data.isEmpty())
        {
            return false;
        }

        qDebug() << "DockWidgetUtils::processSavedState";
        QByteArray sd = data;
        QDataStream stream(&sd, QIODeviceBase::ReadOnly);
        int m, v;

        stream >> m >> v;

        if (stream.status() != QDataStream::Ok || m != VersionMarkers::VersionMarker || v != 0)
        {
            return false;
        }

        while (!stream.atEnd())
        {
            uchar marker;
            stream >> marker;

            switch (marker)
            {
            case DockAreaLayout::DockWidgetStateMarker:
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
            case DockAreaLayout::FloatingDockWidgetTabMarker:
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
            case ToolBarAreaLayout::ToolBarStateMarker:
            case ToolBarAreaLayout::ToolBarStateMarkerEx:
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
                        if (marker == ToolBarAreaLayout::ToolBarStateMarkerEx)
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
