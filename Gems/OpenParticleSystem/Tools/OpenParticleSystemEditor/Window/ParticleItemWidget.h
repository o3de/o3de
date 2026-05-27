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
#include <QMouseEvent>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <Window/ui_ParticleItemWidget.h>
#include <QTableWidgetItem>
#include <QPainter>
#include <EffectorInspector.h>
#include <Window/ParticleLineWidget.h>
#include <Serializer/ParticleSourceData.h>
#endif

namespace Ui
{
    class particleItemWidget;
}

namespace OpenParticleSystemEditor
{
    class ParticleItemWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        ParticleItemWidget(
            EffectorInspector* pDetail,
            OpenParticle::ParticleSourceData::DetailInfo* detail,
            OpenParticle::ParticleSourceData* sourceData,
            AZStd::string graphParentTitle,
            QWidget* parent = nullptr);
        virtual ~ParticleItemWidget();
        void OnSelected();
        void OnRelease();
        void ClickLineWidget(LineWidgetIndex index);
        void SetLineWidgetIndex();
        void SetLineWidgetParent();
        void mousePressEvent(QMouseEvent* event) override;
        void UpdateParticleName();
        void UpdataClassCheck(AZStd::string className, bool bCheck);
        void SetSoloChecked(bool checked) const;
        void SetDetailChecked(bool checked) const;
        QToolButton* GetBtnParticleName();
        EffectorInspector* m_itemDetail;
        OpenParticle::ParticleSourceData::DetailInfo* m_detail;
        OpenParticle::ParticleSourceData* m_pSourceData;
        AZStd::string m_graphParentTitle;
        bool m_widgetSelect;

    private slots:
        void AddRemoveEmitter(AZStd::string className, bool check, LineWidgetIndex index);
        void OnClickParticleSolo(bool checked);
        void OnClickParticleName(bool bCheck);     
        void OnClickSpawn(bool bCheck);
        void OnClickParticles(bool bCheck);
        void OnClickLocation(bool bCheck);
        void OnClickVelocity(bool bCheck);
        void OnClickColor(bool bCheck);
        void OnClickSize(bool bCheck);
        void OnClickForce(bool bCheck);
        void OnClickLight(bool bCheck);
        void OnClickSubUV(bool bCheck);
        void OnClickEvent(bool bCheck);
        void OnClickRenderer(bool bCheck);

    protected:
        void painEvent(QPaintEvent* event);

    public:
        QScopedPointer<Ui::particleItemWidget> m_ui;
        AZStd::unordered_map<AZStd::string, QCheckBox*> m_classChecks;
    };
} // namespace OpenParticleSystemEditor
