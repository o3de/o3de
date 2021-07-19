/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef INICONFIGURATION_H
#define INICONFIGURATION_H

#if !defined(Q_MOC_RUN)
#include <QDir>
#include <QString>
#include <QCoreApplication>
#endif

/** Reads the bootstrap file for listening port
 */
class IniConfiguration
    : public QObject
{
    Q_OBJECT
public:
    explicit IniConfiguration(QObject* pParent = nullptr);
    virtual ~IniConfiguration();

    // Singleton pattern:
    static const IniConfiguration* Get();

    void parseCommandLine(QStringList cmdLine = QCoreApplication::arguments());
    void readINIConfigFile(QDir dir = qApp->applicationDirPath());
    quint16 listeningPort() const;
    void SetListeningPort(quint16 port);

private:
    quint16 m_listeningPort;
    QString m_userConfigFilePath;
};

#endif // INICONFIGURATION_H
