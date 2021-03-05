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
