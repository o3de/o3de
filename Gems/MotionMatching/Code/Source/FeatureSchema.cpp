/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <Allocators.h>
#include <FeatureSchema.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeatureSchema, MotionMatchAllocator)

    FeatureSchema::~FeatureSchema()
    {
        Clear();
    }

    Feature* FeatureSchema::GetFeature(size_t index) const
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
            AZ_Assert(false, "Cannot add feature. Feature with id '%s' has already been registered.", feature->GetId().ToFixedString().c_str());
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

    Feature* FeatureSchema::FindFeatureById(const AZ::TypeId& featureId) const
    {
        const auto result = m_featuresById.find(featureId);
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
            AZ_Error("Motion Matching", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(typeId);
        if (!classData)
        {
            AZ_Warning("Motion Matching", false, "Can't find class data for this type.");
            return nullptr;
        }

        Feature* featureObject = reinterpret_cast<Feature*>(classData->m_factory->Create(classData->m_name));
        return featureObject;
    }

    AZStd::vector<AZStd::string> FeatureSchema::CollectColumnNames() const
    {
        AZStd::vector<AZStd::string> columnNames;

        for (Feature* feature : m_features)
        {
            const size_t numDimensions = feature->GetNumDimensions();
            for (size_t dimension = 0; dimension < numDimensions; ++dimension)
            {
                columnNames.push_back(feature->GetDimensionName(dimension));
            }
        }

        return columnNames;
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
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &FeatureSchema::m_features, "Features", "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ;
    }
} // namespace EMotionFX::MotionMatching
