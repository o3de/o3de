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

#include "SliceFavorites_precompiled.h"

#include "SliceFavoritesSystemComponent.h"

#include "FavoriteDataModel.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace SliceFavorites
{
    void SliceFavoritesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SliceFavoritesSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SliceFavoritesSystemComponent>("SliceFavorites", "[Adds the ability for users to mark slices as favorites for easy instantiation via context menus in the editor]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SliceFavoritesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SliceFavoritesService", 0x2f8751fa));
    }

    void SliceFavoritesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SliceFavoritesService", 0x2f8751fa));
    }

    void SliceFavoritesSystemComponent::Init()
    {
    }

    void SliceFavoritesSystemComponent::Activate()
    {
        m_dataModel = new FavoriteDataModel();
        SliceFavoritesRequestBus::Handler::BusConnect();
        SliceFavoritesSystemComponentRequestBus::Handler::BusConnect();
    }

    void SliceFavoritesSystemComponent::Deactivate()
    {
        delete m_dataModel;
        m_dataModel = nullptr;

        SliceFavoritesRequestBus::Handler::BusDisconnect();
        SliceFavoritesSystemComponentRequestBus::Handler::BusDisconnect();
    }

    size_t SliceFavoritesSystemComponent::GetNumFavorites()
    {
        return m_dataModel->GetNumFavorites();
    }

    void SliceFavoritesSystemComponent::EnumerateFavorites(const AZStd::function<void(const AZ::Data::AssetId& assetId)>& callback)
    {
        m_dataModel->EnumerateFavorites(callback);
    }

    FavoriteDataModel* SliceFavoritesSystemComponent::GetSliceFavoriteDataModel()
    {
        return m_dataModel;
    }
}
