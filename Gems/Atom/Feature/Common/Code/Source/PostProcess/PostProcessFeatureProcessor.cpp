/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

// Using ebus as a temporary workaround
#include <AzFramework/Components/CameraBus.h>

namespace AZ
{
    namespace Render
    {

        void PostProcessFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<PostProcessFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void PostProcessFeatureProcessor::Activate()
        {
            m_currentTime = AZStd::chrono::steady_clock::now();
        }

        void PostProcessFeatureProcessor::Deactivate()
        {
            m_viewAliasMap.clear();
        }

        void PostProcessFeatureProcessor::UpdateTime()
        {
            AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
            AZStd::chrono::duration<float> deltaTime = now - m_currentTime;
            m_currentTime = now;
            m_deltaTime = deltaTime.count();
        }

        void PostProcessFeatureProcessor::SetViewAlias(const AZ::RPI::ViewPtr sourceView, const AZ::RPI::ViewPtr targetView)
        {
            m_viewAliasMap[sourceView.get()] = targetView.get();
        }

        void PostProcessFeatureProcessor::RemoveViewAlias(const AZ::RPI::ViewPtr sourceView)
        {
            m_viewAliasMap.erase(sourceView.get());
        }

        void PostProcessFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "PostProcessFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            UpdateTime();

            if (m_settingsChanged)
            {
                SortPostProcessSettings();
                AggregateLevelSettings();
                m_settingsChanged = false;
            }

            // simulate both the global and each view's post process settings
            // Ideally, every view should be associated to a post process settings. The global
            // setting is returned when a view does not have a post process setting.
            // e.g. Editor Camera, AtomSampleViewer Samples that do not set perViewBlendWeights
            m_globalAggregateLevelSettings->Simulate(m_deltaTime);
            for (auto& settingsPair : m_blendedPerViewSettings)
            {
                settingsPair.second.Simulate(m_deltaTime);
            }
        }

        void PostProcessFeatureProcessor::SortPostProcessSettings()
        {
            // Clear settings from previous frame
            m_sortedFrameSettings.clear();

            // Sort post process settings by layer value and priority
            m_sortedFrameSettings.reserve(m_settings.size());
            for (auto& settings : m_settings)
            {
                m_sortedFrameSettings.push_back(settings.m_postProcessSettings.get());
            }
            AZStd::sort(
                m_sortedFrameSettings.begin(),
                m_sortedFrameSettings.end(),
                [](const PostProcessSettings* lhs, const PostProcessSettings* rhs)-> bool
                {
                    return
                        AZStd::make_pair(lhs->GetLayerCategoryValue(), lhs->GetPriority()) >
                        AZStd::make_pair(rhs->GetLayerCategoryValue(), rhs->GetPriority());
                }
            );
        }

        void PostProcessFeatureProcessor::AggregateLevelSettings()
        {
            // Remove outdated level settings aggregate
            m_globalAggregateLevelSettings = AZStd::make_unique<PostProcessSettings>(this);
            m_blendedPerViewSettings.clear();
            // Apply settings from priority sorted list of level settings
            AZStd::vector_set<const RPI::View*> activeViews;
            for (auto& settings : m_sortedFrameSettings)
            {
                // Apply settings that are not associated with views
                if (settings->m_perViewBlendWeights.empty())
                {
                    settings->ApplySettingsTo(m_globalAggregateLevelSettings.get());
                }

                // Create a setting for each view
                for (auto& viewWeightPair : settings->m_perViewBlendWeights)
                {
                    // create a post process setting if it doesn't exist
                    AZ::RPI::View* view = viewWeightPair.first;
                    activeViews.insert(view);

                    auto viewSettngsIterator = m_blendedPerViewSettings.find(view);
                    if (viewSettngsIterator == m_blendedPerViewSettings.end())
                    {
                        viewSettngsIterator = m_blendedPerViewSettings.insert(
                            AZStd::pair<AZ::RPI::View*, PostProcessSettings>(
                                view,
                                PostProcessSettings(this))).first;
                    }
                    // apply settings
                    float blendWeight = settings->GetBlendWeightForView(viewWeightPair.first);
                    auto& viewSettings = viewSettngsIterator->second;
                    settings->ApplySettingsTo(&viewSettings, blendWeight);
                }
            }

            //RemoveOutdatedViewSettings(activeViews);
        }

        void PostProcessFeatureProcessor::RemoveOutdatedViewSettings(const AZStd::vector_set<const RPI::View*>& activeViews)
        {
            for (auto perViewSetting = m_blendedPerViewSettings.begin(); perViewSetting != m_blendedPerViewSettings.end(); )
            {
                if (AZStd::find(activeViews.begin(), activeViews.end(), perViewSetting->first) == activeViews.end())
                {
                    perViewSetting = m_blendedPerViewSettings.erase(perViewSetting);
                }
                else
                {
                    ++perViewSetting;
                }
            }
        }

        void PostProcessFeatureProcessor::OnPostProcessSettingsChanged()
        {
            m_settingsChanged = true;
        }

        PostProcessSettingsInterface* PostProcessFeatureProcessor::GetSettingsInterface(EntityId entityId)
        {
            for (EntitySettingsEntry& entry : m_settings)
            {
                if (entry.m_entityId == entityId)
                {
                    return entry.m_postProcessSettings.get();
                }
            }
            return nullptr;
        }

        PostProcessSettingsInterface* PostProcessFeatureProcessor::GetOrCreateSettingsInterface(EntityId entityId)
        {
            // Check for settings already registered with entity ID
            PostProcessSettingsInterface* settingsInterface = GetSettingsInterface(entityId);
            if (settingsInterface)
            {
                return settingsInterface;
            }

            // Create new post process settings
            EntitySettingsEntry newEntry;
            newEntry.m_entityId = entityId;
            newEntry.m_postProcessSettings = AZStd::make_unique<PostProcessSettings>(this);

            settingsInterface = newEntry.m_postProcessSettings.get();

            m_settings.push_back(AZStd::move(newEntry));

            return settingsInterface;
        }

        void PostProcessFeatureProcessor::RemoveSettingsInterface(EntityId entityId)
        {
            for (auto iter = m_settings.begin(); iter < m_settings.end(); ++iter)
            {
                if (iter->m_entityId == entityId)
                {
                    m_settings.erase(iter);
                    break;
                }
            }
        }

        AZ::Render::PostProcessSettings* PostProcessFeatureProcessor::GetLevelSettingsFromView(AZ::RPI::ViewPtr view)
        {
            // check for view aliases first
            auto viewAliasiterator = m_viewAliasMap.find(view.get());

            // Use the view alias if it exists
            auto settingsIterator = m_blendedPerViewSettings.find(viewAliasiterator != m_viewAliasMap.end() ? viewAliasiterator->second : view.get());
            // If no settings for the view is found, the global settings is returned.
            return settingsIterator != m_blendedPerViewSettings.end()
                ? &settingsIterator->second
                : m_globalAggregateLevelSettings.get();
        }
    } // namespace Render
} // namespace AZ
