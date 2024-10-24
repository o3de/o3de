/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PostProcess/PostProcessSettings.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Base.h>
#include <AtomCore/std/containers/vector_set.h>
#include <AzCore/std/chrono/chrono.h>


namespace AZ
{
    namespace Render
    {
        //! Feature processor for owning and managing post process settings
        class PostProcessFeatureProcessor final
            : public PostProcessFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PostProcessFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::PostProcessFeatureProcessor, "{A6A8357C-5A34-4297-B4DD-A1FB6556CE3E}", AZ::Render::PostProcessFeatureProcessorInterface);
            static void Reflect(AZ::ReflectContext* context);

            PostProcessFeatureProcessor() = default;
            virtual ~PostProcessFeatureProcessor() = default;

            //! FeatureProcessor overrides...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            //! PostProcessFeatureProcessorInterface...
            PostProcessSettingsInterface* GetSettingsInterface(EntityId entityId) override;
            PostProcessSettingsInterface* GetOrCreateSettingsInterface(EntityId entityId) override;
            void RemoveSettingsInterface(EntityId entityId) override;
            void OnPostProcessSettingsChanged() override;
            PostProcessSettings* GetLevelSettingsFromView(AZ::RPI::ViewPtr view);
            void SetViewAlias(const AZ::RPI::ViewPtr sourceView, const AZ::RPI::ViewPtr targetView) override;
            void RemoveViewAlias(const AZ::RPI::ViewPtr sourceView) override;

        private:
            PostProcessFeatureProcessor(const PostProcessFeatureProcessor&) = delete;

            void UpdateTime();

            // Sorts all post processing settings into buckets based on category (level,
            void SortPostProcessSettings();

            // Aggregates all level settings into a single level setting based their priorities and override settings
            void AggregateLevelSettings();
            void RemoveOutdatedViewSettings(const AZStd::vector_set<const RPI::View*>& activeViews);

            // Members...

            static constexpr const char* FeatureProcessorName = "PostProcessFeatureProcessor";

            struct EntitySettingsEntry
            {
                EntityId m_entityId;
                AZStd::unique_ptr<PostProcessSettings> m_postProcessSettings = nullptr;
            };

            // List of all owned post process settings with corresponding entity ID
            AZStd::vector<EntitySettingsEntry> m_settings;

            // Settings sorted by category and then by priority
            AZStd::vector<PostProcessSettings*> m_sortedFrameSettings;

            // A blended aggregate of all the level settings based on each level setting's priority and override values
            AZStd::unique_ptr<PostProcessSettings> m_globalAggregateLevelSettings = nullptr;

            // Whether owned post process settings have been changed.
            bool m_settingsChanged = true;

            // Time...
            AZStd::chrono::steady_clock::time_point m_currentTime;
            float m_deltaTime;

            // Each camera/view will have its own PostProcessSettings
            AZStd::unordered_map<AZ::RPI::View*, PostProcessSettings> m_blendedPerViewSettings;
            // This is used for mimicking a postfx setting of a different view
            AZStd::unordered_map<AZ::RPI::View*, AZ::RPI::View*> m_viewAliasMap;
        };
    } // namespace Render
} // namespace AZ
