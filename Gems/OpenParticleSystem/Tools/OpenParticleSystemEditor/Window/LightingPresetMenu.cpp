/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LightingPresetMenu.h>
#include <Viewport/OpenParticleViewportRequestBus.h>
#include <Atom/Feature/Utils/LightingPreset.h>

namespace OpenParticleSystemEditor
{
    LightingPresetMenu::LightingPresetMenu(const QString& title, QWidget* parent)
        : QMenu(title, parent)
    {
        connect(
            this, static_cast<void (LightingPresetMenu::*)(const int)>(&LightingPresetMenu::ActionIndexChange), this,
            [this](int index)
            {
                if (index >= 0 && index < m_presetNameSet.size())
                {
                    int i = 0;
                    AZStd::string name;
                    for (auto it : m_presetNameSet)
                    {
                        if (i == index)
                        {
                            name = it;
                            break;
                        }
                        i++;
                    }
                    OpenParticleViewportRequestBus::Broadcast(&OpenParticleViewportRequestBus::Events::SelectLightingPresetByName, name);
                }
            });

        Refresh();

        OpenParticleViewportNotificationBus::Handler::BusConnect();
    }

    LightingPresetMenu::~LightingPresetMenu()
    {
        OpenParticleViewportNotificationBus::Handler::BusDisconnect();
    }

    void LightingPresetMenu::Refresh()
    {
        clear();

        m_presetNameSet.clear();
        m_actions.clear();
        OpenParticleViewportRequestBus::BroadcastResult(m_presetNameSet, &OpenParticleViewportRequestBus::Events::GetLightingPresets);
        if (m_presetNameSet.empty())
        {
            EBUS_EVENT(OpenParticleViewportRequestBus, ReloadContent);
            EBUS_EVENT_RESULT(m_presetNameSet, OpenParticleViewportRequestBus, GetLightingPresets);
        }

        blockSignals(true);
        for (const auto& preset : m_presetNameSet)
        {
            QAction* action = new QAction(tr(preset.data()), this);
            connect(action, &QAction::triggered, this, &LightingPresetMenu::ClickAction);
            action->setCheckable(true);
            addAction(action);
            m_actions.push_back(action);
        }
        blockSignals(false);

        AZStd::string preset;
        OpenParticleViewportRequestBus::BroadcastResult(
            preset, &OpenParticleViewportRequestBus::Events::GetLightingPresetSelection);
        OnLightingPresetSelected(preset);
    }

    void LightingPresetMenu::OnLightingPresetSelected(AZStd::string preset)
    {
        auto it = m_presetNameSet.find(preset);
        if (it != m_presetNameSet.end())
        {
            auto index = AZStd::distance(m_presetNameSet.begin(), it);
            m_actions[index]->setChecked(true);
        }
    }

    void LightingPresetMenu::OnLightingPresetAdded([[maybe_unused]]AZ::Render::LightingPresetPtr preset)
    {
        if (!m_reloading)
        {
            Refresh();
        }
    }

    void LightingPresetMenu::OnLightingPresetChanged([[maybe_unused]]AZ::Render::LightingPresetPtr preset)
    {
    }

    void LightingPresetMenu::OnBeginReloadContent()
    {
        m_reloading = true;
    }

    void LightingPresetMenu::OnEndReloadContent()
    {
        m_reloading = false;
        Refresh();
    }

    void LightingPresetMenu::ClickAction()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        AZStd::string str = action->text().toUtf8().data();
        int index = 0;
        for (const auto& prest : m_presetNameSet)
        {
            if (str.compare(prest.data()) == 0)
            {
                break;
            }
            ++index;
        }
        emit ActionIndexChange(index);

        for (const auto& vec : m_actions)
        {
            vec->setChecked(false);
        }
        m_actions[index]->setChecked(true);
    }

} // namespace OpenParticleSystemEditor

#include <moc_LightingPresetMenu.cpp>
