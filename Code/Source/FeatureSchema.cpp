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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <Allocators.h>
#include <FeatureSchema.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeatureSchema, MotionMatchAllocator, 0)

    const Feature* FeatureSchema::GetFeature(size_t index) const
    {
        return m_features[index];
    }

    const AZStd::vector<Feature*>& FeatureSchema::GetFeatures() const
    {
        return m_features;
    }

    void FeatureSchema::AddFeature(Feature* feature)
    {
        // Try to see if there is a feature with the same id already.
        auto iterator = AZStd::find_if(m_featuresById.begin(), m_featuresById.end(), [&feature](const auto& curEntry) -> bool {
            return (feature->GetId() == curEntry.second->GetId());
            });

        if (iterator != m_featuresById.end())
        {
            AZ_Assert(false, "Cannot add feature. Feature with id '%s' has already been registered.", feature->GetId().data);
            return;
        }

        m_featuresById.emplace(feature->GetId(), feature);
        m_features.emplace_back(feature);
    }

    void FeatureSchema::Clear()
    {
        for (Feature* feature : m_features)
        {
            delete feature;
        }
        m_featuresById.clear();
        m_features.clear();
    }

    size_t FeatureSchema::GetNumFeatures() const
    {
        return m_features.size();
    }

    Feature* FeatureSchema::FindFeatureById(const AZ::TypeId& featureTypeId) const
    {
        const auto result = m_featuresById.find(featureTypeId);
        if (result == m_featuresById.end())
        {
            return nullptr;
        }

        return result->second;
    }

    Feature* FeatureSchema::CreateFeatureByType(const AZ::TypeId& typeId)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(typeId);
        if (!classData)
        {
            AZ_Warning("EMotionFX", false, "Can't find class data for this type.");
            return nullptr;
        }

        Feature* featureObject = reinterpret_cast<Feature*>(classData->m_factory->Create(classData->m_name));
        return featureObject;
    }

    void FeatureSchema::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<FeatureSchema>()
            ->Version(1)
            ->Field("features", &FeatureSchema::m_features);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<FeatureSchema>("FeatureSchema", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &FeatureSchema::m_features, "Features", "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ;
    }
} // namespace EMotionFX::MotionMatching
