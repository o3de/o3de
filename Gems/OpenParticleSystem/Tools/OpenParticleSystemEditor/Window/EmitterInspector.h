/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#if !defined(Q_MOC_RUN)
#include <QByteArray>
#include <QWidget>
#include <Window/ui_EmitterInspector.h>
#include <Window/ParticleGraphicsScence.h>
#include <QtWidgets/qtwidgetsglobal.h>
#include <QCoreApplication>
#include <QtGui/qwindowdefs.h>
#include <QPoint>
#include <QSize>
#include <QCursor>
#include <QGuiApplication>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <EffectorInspector.h>
#include <Document/ParticleDocumentBus.h>
#endif

namespace Ui
{
    class EmitterInspector;
}

namespace OpenParticleSystemEditor
{
    class EmitterInspector
        : public QWidget
        , public ParticleDocumentNotifyBus::Handler
    {
        Q_OBJECT
    public:
        EmitterInspector(EffectorInspector* pViewDetail, QWidget* parent = nullptr, QString windowTitle = "");
        virtual ~EmitterInspector();
        void OnParticleSourceDataLoaded(OpenParticle::ParticleSourceData* particleSourceData, AZStd::string particleAssetPath) const override;

    private:
        QScopedPointer<Ui::EmitterInspector> m_ui;
        ParticleGraphicsScence* scene;

        QString m_assetPath = "";
    };
} // namespace OpenParticleSystemEditor
