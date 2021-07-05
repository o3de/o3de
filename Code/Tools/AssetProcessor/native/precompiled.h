/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETPROCESSOR_PRECOMPILED_H
#define ASSETPROCESSOR_PRECOMPILED_H

#if 1 // change to 0 to temporarily turn off pch to make sure includes are ok

#include <QObject>
#include <QMetaObject>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QList>
#if !defined(BATCH_MODE)
#include <QApplication>
#endif
#include <QHash>
#include <QByteArray>
#include <QAbstractListModel>
#include <QSet>
#include <QDir>
#include <QDebug>
#include <QPair>
#include <QTimer>
#include <QQueue>
#include <QTime>
#include <QVector>
#include <QThread>
#include <QProcess>
#include <QCoreApplication>

#endif

#endif // ASSETPROCESSOR_PRECOMPILED_H
