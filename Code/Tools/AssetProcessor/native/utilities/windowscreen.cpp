/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QGuiApplication>
#include <QSettings>
#include <QScreen>

#include "windowscreen.h"

WindowScreen::WindowScreen(QObject* parent)
    : QObject(parent)
{
}

WindowScreen::~WindowScreen()
{
}


void WindowScreen::loadSettings(int width, int height, int minimumWidth, int minimumHeight)
{
    QSettings loader;
    setPositionX(loader.value(m_windowName + "/" + "PositionX", 99999).toInt());
    setPositionY(loader.value(m_windowName + "/" + "PositionY", 99999).toInt());
    setWidth(loader.value(m_windowName + "/" + "Width", width).toInt());
    setHeight(loader.value(m_windowName + "/" + "Height", height).toInt());
    setWindowState(static_cast<QWindow::Visibility>(loader.value(m_windowName + "/" + "WindowState", QWindow::Windowed).toInt()));

    if (!CheckSettings(minimumWidth, minimumHeight))
    {
        CenterWindowInPrimaryScreen(minimumWidth, minimumHeight);
    }
}

void WindowScreen::saveSettings()
{
    QSettings saver;
    saver.remove(m_windowName + "/" + "WindowState");
    saver.setValue(m_windowName + "/" + "WindowState", m_windowCurrentInfo.m_windowState);

    //If the window current state is maximised or fullscreen than we save x,y,width and height
    //of the window prev state so that it can be minimised correctly the next time we start the application
    WindowScreenInfo* screenInfo = nullptr;
    if (m_windowCurrentInfo.m_windowState != QWindow::Maximized && m_windowCurrentInfo.m_windowState != QWindow::FullScreen)
    {
        screenInfo = &m_windowCurrentInfo;
    }
    else
    {
        screenInfo = &m_windowPreviousInfo;
    }

    saver.remove(m_windowName + "/" + "PositionX");
    saver.setValue(m_windowName + "/" + "PositionX", screenInfo->m_positionX);
    saver.remove(m_windowName + "/" + "PositionY");
    saver.setValue(m_windowName + "/" + "PositionY", screenInfo->m_positionY);
    saver.remove(m_windowName + "/" + "Width");
    saver.setValue(m_windowName + "/" + "Width", screenInfo->m_width);
    saver.remove(m_windowName + "/" + "Height");
    saver.setValue(m_windowName + "/" + "Height", screenInfo->m_height);
}

bool WindowScreen::CheckSettings(int minimumWidth, int minimumHeight)
{
    if (m_windowCurrentInfo.m_positionX == 99999 && m_windowCurrentInfo.m_positionY == 99999)
    {
        //we are running the app first time and there is no settings to load from,
        //we will centre the window ourselves
        return false;
    }

    if (m_windowCurrentInfo.m_width < minimumWidth || m_windowCurrentInfo.m_height < minimumHeight)
    {
        return false;
    }

    //Check whether the window is fully inside the display
    QScreen* screen = QGuiApplication::primaryScreen();

    bool isPosXOK =  m_windowCurrentInfo.m_positionX >= (screen->availableVirtualGeometry().x()) && m_windowCurrentInfo.m_positionX <= (screen->availableVirtualGeometry().width()) ? true : false;
    bool isPosYOK =  m_windowCurrentInfo.m_positionY >= (screen->availableVirtualGeometry().y()) && m_windowCurrentInfo.m_positionY <= (screen->availableVirtualGeometry().height()) ? true : false;
    bool isWidthOK = m_windowCurrentInfo.m_positionX + m_windowCurrentInfo.m_width <= screen->availableVirtualSize().width() ? true : false;
    bool isHeightOK = m_windowCurrentInfo.m_positionY + m_windowCurrentInfo.m_height <= screen->availableVirtualSize().height() ? true : false;

    return isPosXOK && isPosYOK && isWidthOK && isHeightOK;
}

void WindowScreen::CenterWindowInPrimaryScreen(int minimumWidth, int minimumHeight)
{
    //check to see whether the width is more than minimum width
    if (m_windowCurrentInfo.m_width < minimumWidth)
    {
        m_windowCurrentInfo.m_width = minimumWidth;
    }

    //check to see whether the height is more than minimum height
    if (m_windowCurrentInfo.m_height < minimumHeight)
    {
        m_windowCurrentInfo.m_height = minimumHeight;
    }

    QScreen* screen = QGuiApplication::primaryScreen();
    m_windowCurrentInfo.m_positionX = qRound((screen->availableGeometry().width() - screen->availableGeometry().x()) / 2.0 - m_windowCurrentInfo.m_width / 2.0);
    m_windowCurrentInfo.m_positionY = qRound((screen->availableGeometry().height() - screen->availableGeometry().y()) / 2.0 - m_windowCurrentInfo.m_height / 2.0);
    m_windowCurrentInfo.m_windowState = QWindow::Windowed;
}
int WindowScreen::positionX() const
{
    return m_windowCurrentInfo.m_positionX;
}

void WindowScreen::setPositionX(int posX)
{
    if (posX == m_windowCurrentInfo.m_positionX)
    {
        return;
    }
    m_windowPreviousInfo.m_positionX = m_windowCurrentInfo.m_positionX;
    m_windowCurrentInfo.m_positionX = posX;


    Q_EMIT positionXChanged();
}
int WindowScreen::positionY() const
{
    return m_windowCurrentInfo.m_positionY;
}

void WindowScreen::setPositionY(int posY)
{
    if (posY == m_windowCurrentInfo.m_positionY)
    {
        return;
    }
    m_windowPreviousInfo.m_positionY = m_windowCurrentInfo.m_positionY;
    m_windowCurrentInfo.m_positionY = posY;

    Q_EMIT positionYChanged();
}
int WindowScreen::width() const
{
    return m_windowCurrentInfo.m_width;
}

void WindowScreen::setWidth(int width)
{
    if (width == m_windowCurrentInfo.m_width)
    {
        return;
    }
    m_windowPreviousInfo.m_width = m_windowCurrentInfo.m_width;
    m_windowCurrentInfo.m_width = width;

    Q_EMIT widthChanged();
}
int WindowScreen::height() const
{
    return m_windowCurrentInfo.m_height;
}

void WindowScreen::setHeight(int height)
{
    if (height == m_windowCurrentInfo.m_height)
    {
        return;
    }

    m_windowPreviousInfo.m_height = m_windowCurrentInfo.m_height;
    m_windowCurrentInfo.m_height = height;

    Q_EMIT heightChanged();
}

QString WindowScreen::windowName() const
{
    return m_windowName;
}

void WindowScreen::setWindowName(QString windowName)
{
    m_windowName = windowName;
}
QWindow::Visibility WindowScreen::windowState() const
{
    return m_windowCurrentInfo.m_windowState;
}

void WindowScreen::setWindowState(QWindow::Visibility state)
{
    if (m_windowCurrentInfo.m_windowState == state)
    {
        return;
    }
    m_windowPreviousInfo.m_windowState = m_windowCurrentInfo.m_windowState;
    m_windowCurrentInfo.m_windowState = state;

    Q_EMIT windowStateChanged();
}

