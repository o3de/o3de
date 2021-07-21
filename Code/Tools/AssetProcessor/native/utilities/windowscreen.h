/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef WINDOWSCREEN_H
#define WINDOWSCREEN_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QString>
#include <QWindow>
#endif


/** The WindowScreenInfo struct stores the x, y, width, height and
 * other window state info.
 */
struct WindowScreenInfo
{
    int m_positionX = 0;
    int m_positionY = 0;
    int m_width = 0;
    int m_height = 0;
    QWindow::Visibility m_windowState = QWindow::Windowed;
};

/** The WindowScreen class is responsible for storing information about the
 * application window.
 */
class WindowScreen
    : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int positionX READ positionX WRITE setPositionX NOTIFY positionXChanged)
    Q_PROPERTY(int positionY READ positionY WRITE setPositionY NOTIFY positionYChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(QWindow::Visibility windowState READ windowState WRITE setWindowState NOTIFY windowStateChanged)
    Q_PROPERTY(QString windowName READ windowName WRITE setWindowName)
public:

    /// standard Qt constructor
    explicit WindowScreen(QObject* parent = 0);
    virtual ~WindowScreen();

    ///Loads settings
    Q_INVOKABLE void loadSettings(int width, int heigth, int minimumWidth, int minimumHeight);

    ///save settings
    Q_INVOKABLE void saveSettings();


    int positionX() const;
    void setPositionX(int positionX);

    int positionY() const;
    void setPositionY(int positionY);

    int width() const;
    void setWidth(int width);

    int height() const;
    void setHeight(int height);

    bool isMaximize() const;
    void setIsMaximize(bool isMaximize);

    QString windowName() const;
    void setWindowName(QString windowName);

    QWindow::Visibility windowState() const;
    void setWindowState(QWindow::Visibility state);

Q_SIGNALS:
    void positionXChanged();
    void positionYChanged();
    void widthChanged();
    void heightChanged();
    void windowStateChanged();

private:

    ///Check to see whether settings are valid or not
    bool CheckSettings(int minimumWidth, int minimumHeight);

    ///Centers the window in primary screen
    void CenterWindowInPrimaryScreen(int minimumWidth, int minimumHeight);

    WindowScreenInfo m_windowCurrentInfo;
    WindowScreenInfo m_windowPreviousInfo;
    QString m_windowName = QString();
};


#endif // WINDOWSCREEN_H
