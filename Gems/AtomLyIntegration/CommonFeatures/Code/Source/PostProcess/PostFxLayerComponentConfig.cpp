/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerCategoriesProviderRequestBus.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace Render
    {
        void PostFxLayerComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PostFxLayerComponentConfig, ComponentConfig>()
                    ->Version(2)
                    ->Field("layerCategory", &PostFxLayerComponentConfig::m_layerCategoryValue)

#define SERIALIZE_CLASS PostFxLayerComponentConfig
#include <Atom/Feature/ParamMacros/StartParamSerializeContext.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS

                    ->Field("cameraTags", &PostFxLayerComponentConfig::m_cameraTags)
                    ->Field("exclusionTags", &PostFxLayerComponentConfig::m_exclusionTags)
                    ;
            }
        }

        void PostFxLayerComponentConfig::CopySettingsTo(PostProcessSettingsInterface* settings) const
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_TARGET

            settings->SetLayerCategoryValue(m_layerCategoryValue);
        }

        AZStd::string PostFxLayerComponentConfig::GetPriorityLabel()
        {
            auto priorityLayersList = BuildLayerCategories();

            // use the first match as the priority label since all labels with the same priority will belong to the same layer
            auto it = AZStd::find_if(
                priorityLayersList.begin(),
                priorityLayersList.end(),
                [this](const auto& entry) { return m_layerCategoryValue == entry.first; });

            return it == priorityLayersList.end() ? "Priority" : "Priority in " + it->second;
        }

        AZStd::vector<AZStd::pair<int, AZStd::string>> PostFxLayerComponentConfig::BuildLayerCategories() const
        {
            // Call the bus to retrieve a list of categories
            PostFx::LayerCategoriesMap layerCategories;
            PostFxLayerCategoriesProviderRequestBus::Broadcast(&PostFxLayerCategoriesProviderRequests::GetLayerCategories, layerCategories);

            AZStd::vector<AZStd::pair<int, AZStd::string>> results;
            results.reserve(layerCategories.size());
            for (const auto& layer : layerCategories)
            {
                results.push_back(AZStd::pair<int, AZStd::string>(layer.second, layer.first));
            }
            AZStd::sort(
                results.begin(),
                results.end(),
                [](auto& left, auto& right) { return left.first < right.first;});
            return results;
        }
    } // namespace Render
} // namespace AZ
