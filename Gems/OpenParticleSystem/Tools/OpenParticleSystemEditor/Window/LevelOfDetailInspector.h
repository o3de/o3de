/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Serializer/ParticleSourceData.h>
#include <Document/ParticleDocumentBus.h>
#include <Window/LevelOfDetailInspectorNotifyBus.h>
#endif

#include <QWidget>
#include <QMenu>
#include <QPoint>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QString>
#include <QToolBox>
#include <QEvent>

namespace OpenParticleSystemEditor
{
    class LevelWidget : public QWidget
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(LevelWidget, AZ::SystemAllocator, 0);

        explicit LevelWidget(QWidget* parent = nullptr);
        ~LevelWidget();

        void SetIndex(AZ::u32 index);
        AZ::u32 GetIndex();
        void SetDistance(float distance);
        void AddLevelItem(AZ::u32 emitterIndex, bool checked, AZStd::string& name);
        void RemoveItem(AZ::u32 index);
        AZStd::vector<QCheckBox*> m_checkboxes;
    Q_SIGNALS:
        void RemoveLevelItem(LevelWidget* levelWidget);
        void OnEditingFinished(float distance);
        void OnEmitterChecked(AZ::u32 index, bool checked);

    protected:
        bool eventFilter(QObject* obj, QEvent* ev) override;

    private:
        void SetupUi();
        void ConnectWidget();

        QLabel* m_label = nullptr;
        QLineEdit* m_lineEdit = nullptr;
        QPushButton* m_deleteButton = nullptr;
        QPushButton* m_expandButton = nullptr;
        QVBoxLayout* m_layout = nullptr;
        QWidget* m_itemsWidget = nullptr;

        AZ::u32 m_indexOfLod = 0;
    };

    class LevelOfDetailInspector
        : public QWidget
        , public ParticleDocumentNotifyBus::Handler
        , public LevelOfDetailInspectorNotifyBus::Handler
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(LevelOfDetailInspector, AZ::SystemAllocator, 0);

        explicit LevelOfDetailInspector(QWidget* parent = nullptr);
        ~LevelOfDetailInspector();

        void OnParticleSourceDataLoaded([[maybe_unused]] OpenParticle::ParticleSourceData* particleSourceData, [[maybe_unused]] AZStd::string particleAssetPath) const override;
        void ReloadLevel([[maybe_unused]] AZStd::string widgetName) override;

    private slots:
        void OnAddLevel();
        void OnRemoveLevel(LevelWidget * levelWidget);

    private:
        void SetupUi();
        void AddLevel(OpenParticle::ParticleSourceData* sourceData, AZ::u32 indexOfLod);
        void ClearLevels();
        void OnDistanceModified(AZ::u32 index, float distance);

        QVBoxLayout* m_layout = nullptr;
        QPushButton* m_addLevelOfDetail = nullptr;

        AZStd::vector<LevelWidget*> m_levels;
        AZStd::string m_widgetName;
    };
} // namespace OpenParticleSystemEditor
