/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LevelOfDetailInspector.h>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QScrollArea>
#include <QCheckBox>
#include <QMessageBox>

namespace OpenParticleSystemEditor
{
    const int SPACING = 20;
    LevelOfDetailInspector::LevelOfDetailInspector(QWidget* parent)
        : QWidget(parent)
    {
        SetupUi();

        ParticleDocumentNotifyBus::Handler::BusConnect();
        LevelOfDetailInspectorNotifyBus::Handler::BusConnect();
    }

    void LevelOfDetailInspector::SetupUi()
    {
        m_layout = new QVBoxLayout();
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
        m_addLevelOfDetail = new QPushButton(tr("+Add Level of Detail"), this);
        connect(m_addLevelOfDetail, SIGNAL(clicked()), this, SLOT(OnAddLevel()));
        m_addLevelOfDetail->setEnabled(false);
        m_layout->addWidget(m_addLevelOfDetail);
        m_layout->setAlignment(Qt::AlignTop);

        auto widget = new QWidget();
        widget->setLayout(m_layout);
        auto scrollArea = new QScrollArea();
        scrollArea->setWidget(widget);
        scrollArea->setWidgetResizable(true);
        auto layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scrollArea);

        setLayout(layout);
    }
    
    LevelOfDetailInspector::~LevelOfDetailInspector()
    {
        ParticleDocumentNotifyBus::Handler::BusDisconnect();
        LevelOfDetailInspectorNotifyBus::Handler::BusDisconnect();
        ClearLevels();
    }

    void LevelOfDetailInspector::AddLevel(OpenParticle::ParticleSourceData* sourceData, AZ::u32 indexOfLod)
    {
        auto levelWidget = new LevelWidget();
        levelWidget->SetIndex(static_cast<AZ::u32>(indexOfLod));
        auto& lod = sourceData->m_lods.at(indexOfLod);
        levelWidget->SetDistance(lod.m_distance);
        connect(levelWidget, &LevelWidget::OnEditingFinished, this, [this, levelWidget](float distance)
            {
                auto idx = levelWidget->GetIndex();
                OnDistanceModified(idx, distance);
            });
        connect(levelWidget, &LevelWidget::RemoveLevelItem, this, &LevelOfDetailInspector::OnRemoveLevel);
        connect(levelWidget, &LevelWidget::OnEmitterChecked, this, [levelWidget, this](AZ::u32 index, bool checked)
            {
                OpenParticle::ParticleSourceData* srcData = nullptr;
                EBUS_EVENT_ID_RESULT(srcData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
                AZ_Assert(srcData != nullptr, "Can't get valid source data");
                auto idx = levelWidget->GetIndex();
                auto& lod = srcData->m_lods.at(idx);

                auto it = AZStd::find(lod.m_emitters.begin(), lod.m_emitters.end(), index);
                if (checked)
                {
                    if (it == lod.m_emitters.end())
                    {
                        lod.m_emitters.push_back(index);
                    }
                }
                else
                {
                    if (it != lod.m_emitters.end())
                    {
                        lod.m_emitters.erase(it);
                    }
                }
                EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            });
        AZ::u32 index = 0;
        for (const auto& emitter : sourceData->m_emitters)
        {
            auto iter = AZStd::find(lod.m_emitters.begin(), lod.m_emitters.end(), index);
            bool checked = (iter == lod.m_emitters.end()) ? false : true;
            levelWidget->AddLevelItem(index, checked, emitter->m_name);
            index++;
        }
        m_layout->addWidget(levelWidget);
        m_levels.push_back(levelWidget);
    }

    void LevelOfDetailInspector::OnDistanceModified(AZ::u32 index, float distance)
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        auto& lods = sourceData->m_lods;
        for (AZ::u32 i = 0; i < lods.size(); i++)
        {
            if (i == index)
            {
                continue;
            }
            if (qFuzzyCompare(lods[i].m_distance, distance))
            {
                QMessageBox::information(this, tr("Warning"), tr("Invalid distance!"));
                auto iter = AZStd::find_if(m_levels.begin(), m_levels.end(), [index](const auto& widget)
                    {
                        return widget->GetIndex() == index ? true : false;
                    });
                if (iter != m_levels.end())
                {
                    // restore distance
                    (*iter)->SetDistance(lods[index].m_distance);
                }
                return;
            }
        }

        auto& lod = lods[index];
        lod.m_distance = distance;
        AZStd::sort(lods.begin(), lods.end(), [](const auto& first, const auto& second)
            {
                return first.m_distance < second.m_distance ? true : false;
            });
        ReloadLevel(m_widgetName);
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void LevelOfDetailInspector::OnAddLevel()
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData == nullptr)
        {
            return;
        }
        auto& lod = sourceData->m_lods.emplace_back();
        lod.m_distance = 0;
        AddLevel(sourceData, static_cast<AZ::u32>(sourceData->m_lods.size() - 1));
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void LevelOfDetailInspector::OnRemoveLevel(LevelWidget* levelWidget)
    {
        AZ::u32 index = 0;
        auto it = AZStd::find(m_levels.begin(), m_levels.end(), levelWidget);
        if (it != m_levels.end())
        {
            index = static_cast<AZ::u32>(it - m_levels.begin());
            m_levels.erase(it);
        }
        m_layout->removeWidget(levelWidget);
        delete levelWidget;

        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData != nullptr)
        {
            sourceData->m_lods.erase(sourceData->m_lods.begin() + index);
        }

        for (auto i = 0; i < m_levels.size(); i++)
        {
            m_levels[i]->SetIndex(i);
        }
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void LevelOfDetailInspector::OnParticleSourceDataLoaded([[maybe_unused]] OpenParticle::ParticleSourceData* particleSourceData, [[maybe_unused]]AZStd::string particleAssetPath) const
    {
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(particleAssetPath.c_str(), fileName);

        EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, fileName);
    }

    void LevelOfDetailInspector::ClearLevels()
    {
        for (auto& levelWidget : m_levels)
        {
            m_layout->removeWidget(levelWidget);
            delete levelWidget;
            levelWidget = nullptr;
        }
        m_levels.clear();
    }

    void LevelOfDetailInspector::ReloadLevel([[maybe_unused]] AZStd::string widgetName)
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        m_widgetName = widgetName;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData == nullptr || sourceData->m_lods.size() == 0 || sourceData->m_details.size() == 0)
        {
            ClearLevels();
            m_addLevelOfDetail->setEnabled((sourceData == nullptr || sourceData->m_details.size() == 0) ? false : true);
            return;
        }
        m_addLevelOfDetail->setEnabled(true);
        for (AZ::u32 i = 0; i < sourceData->m_lods.size(); i++)
        {
            if (m_levels.size() <= i)
            {
                AddLevel(sourceData, i);
                continue;
            }
            m_levels[i]->SetIndex(static_cast<AZ::u32>(i));
            auto& lod = sourceData->m_lods[i];
            m_levels[i]->SetDistance(lod.m_distance);
            auto checkboxCount = m_levels[i]->m_checkboxes.size();

            for (AZ::u32 index = 0; index < sourceData->m_emitters.size(); index++)
            {
                auto iter = AZStd::find(lod.m_emitters.begin(), lod.m_emitters.end(), index);
                bool checked = (iter == lod.m_emitters.end()) ? false : true;
                if (index < checkboxCount)
                {
                    auto checkbox = m_levels[i]->m_checkboxes[index];
                    checkbox->setText(sourceData->m_emitters[index]->m_name.data());
                    checkbox->setChecked(checked);
                }
                else
                {
                    m_levels[i]->AddLevelItem(index, checked, sourceData->m_emitters[index]->m_name);
                }
            }

            if (checkboxCount > sourceData->m_emitters.size())
            {
                while (m_levels[i]->m_checkboxes.size() > sourceData->m_emitters.size())
                {
                    m_levels[i]->RemoveItem(static_cast<AZ::u32>(m_levels[i]->m_checkboxes.size() - 1));
                }
            }
        }

        while (m_levels.size() > sourceData->m_lods.size())
        {
            auto& widget = m_levels[m_levels.size() - 1];
            m_levels.erase(&widget);
            m_layout->removeWidget(widget);
            delete widget;
        }
    }

    LevelWidget::LevelWidget(QWidget* parent)
        : QWidget(parent)
    {
        SetupUi();

        ConnectWidget();
        m_lineEdit->installEventFilter(this);
        m_lineEdit->setFocusPolicy(Qt::ClickFocus);
    }

    LevelWidget::~LevelWidget()
    {
        for (auto& checkbox : m_checkboxes)
        {
            m_layout->removeWidget(checkbox);
            delete checkbox;
            checkbox = nullptr;
        }
        m_checkboxes.clear();
    }

    void LevelWidget::SetupUi()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        QHBoxLayout* hBoxLayout = new QHBoxLayout();
        hBoxLayout->setContentsMargins(0, 0, 0, 0);
        hBoxLayout->setSpacing(0);
        m_deleteButton = new QPushButton();
        m_deleteButton->setIcon(QIcon(":/Gallery/Delete.svg"));
        m_deleteButton->setFixedWidth(SPACING);
        m_label = new QLabel("LOD");
        QSpacerItem* spacer = new QSpacerItem(SPACING, SPACING, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hBoxLayout->addWidget(m_deleteButton);
        hBoxLayout->addWidget(m_label);
        hBoxLayout->addItem(spacer);
        hBoxLayout->setAlignment(Qt::AlignLeft);
        mainLayout->addLayout(hBoxLayout);

        hBoxLayout = new QHBoxLayout();
        auto label = new QLabel(tr("Max Distance"));
        m_lineEdit = new QLineEdit();
        spacer = new QSpacerItem(SPACING, SPACING, QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_lineEdit->setMaximumWidth(150);
        m_lineEdit->setAlignment(Qt::AlignLeft);
        hBoxLayout->addSpacing(SPACING);
        hBoxLayout->addWidget(label);
        hBoxLayout->addWidget(m_lineEdit);
        hBoxLayout->addItem(spacer);
        hBoxLayout->setAlignment(Qt::AlignLeft);
        mainLayout->addLayout(hBoxLayout);

        hBoxLayout = new QHBoxLayout();
        m_expandButton = new QPushButton();
        m_expandButton->setIcon(QIcon(":/Cards/img/UI20/Cards/caret-down.svg"));
        m_expandButton->setFixedWidth(SPACING);
        label = new QLabel(tr("Emitter"));
        hBoxLayout->addSpacing(SPACING);
        hBoxLayout->addWidget(m_expandButton);
        hBoxLayout->addWidget(label);
        mainLayout->addLayout(hBoxLayout);

        m_layout = new QVBoxLayout();
        m_itemsWidget = new QWidget();
        m_layout->setContentsMargins(SPACING, 0, 0, 0);
        m_itemsWidget->setLayout(m_layout);
        m_itemsWidget->setContentsMargins(SPACING, 0, 0, 0);
        mainLayout->addWidget(m_itemsWidget);

        setLayout(mainLayout);
    }

    void LevelWidget::ConnectWidget()
    {
        connect(m_deleteButton, &QPushButton::clicked, this, [this]()
            {
                emit RemoveLevelItem(this);
            });
        connect(m_lineEdit, &QLineEdit::returnPressed, this, [this]()
            {
                if (m_lineEdit->hasFocus())
                {
                    this->setFocus();
                }
            });
        connect(m_expandButton, &QPushButton::clicked, this, [this]()
            {
                bool expand = m_itemsWidget->isVisible();
                if (expand)
                {
                    m_expandButton->setIcon(QIcon(":/Cards/img/UI20/Cards/caret-right.svg"));
                }
                else
                {
                    m_expandButton->setIcon(QIcon(":/Cards/img/UI20/Cards/caret-down.svg"));
                }
                m_itemsWidget->setVisible(!expand);
            });
    }

    void LevelWidget::AddLevelItem(AZ::u32 emitterIndex, bool checked, AZStd::string& name)
    {
        (void)emitterIndex;
        auto checkbox = new QCheckBox(name.data());
        checkbox->setChecked(checked);
        connect(checkbox, &QCheckBox::stateChanged, this, [this, checkbox](int state)
            {
                auto it = AZStd::find_if(m_checkboxes.begin(), m_checkboxes.end(), [checkbox](const auto& item)
                    {
                        return checkbox == item;
                    });
                if (it == m_checkboxes.end())
                {
                    return;
                }
                AZ::u32 index = static_cast<AZ::u32>(it - m_checkboxes.begin());
                bool checked = state == Qt::Checked;
                emit OnEmitterChecked(index, checked);
            });
        m_layout->addWidget(checkbox);
        m_checkboxes.emplace_back(checkbox);
    }

    void LevelWidget::RemoveItem(AZ::u32 index)
    {
        auto& checkbox = m_checkboxes[index];
        m_layout->removeWidget(checkbox);
        m_checkboxes.erase(&checkbox);
        delete checkbox;
    }

    void LevelWidget::SetIndex(AZ::u32 index)
    {
        m_label->setText(QString("LOD %1").arg(index + 1));
        m_indexOfLod = index;
    }
    AZ::u32 LevelWidget::GetIndex()
    {
        return m_indexOfLod;
    }

    void LevelWidget::SetDistance(float distance)
    {
        m_lineEdit->setText(QString("%1").arg(distance));
    }
    bool LevelWidget::eventFilter(QObject* obj, QEvent* ev)
    {
        if (m_lineEdit == obj)
        {
            if (ev->type() == QEvent::FocusOut)
            {
                emit OnEditingFinished(m_lineEdit->text().toFloat());
            }
        }
        return QWidget::eventFilter(obj, ev);
    }
} // namespace OpenParticleSystemEditor

#include <moc_LevelOfDetailInspector.cpp>
