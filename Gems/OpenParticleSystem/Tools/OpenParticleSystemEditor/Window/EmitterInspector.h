/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Document/ParticleDocumentBus.h>
#include <EffectorInspector.h>

#include <QByteArray>
#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QPoint>
#include <QSize>
#include <qtwidgetsglobal.h>
#include <QWidget>
#include <qwindowdefs.h>

#include <Window/ParticleGraphicsScence.h>
#include <Window/ui_EmitterInspector.h>

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
