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

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/map.h>

#include <QMenu>
#include <QSettings>

#include <SliceFavorites/SliceFavoritesBus.h>
#include "SliceFavoritesSystemComponentBus.h"

namespace SliceFavorites
{
    class FavoriteDataModel;

    class SliceFavoritesSystemComponent
        : public AZ::Component
        , private SliceFavoritesRequestBus::Handler
        , private SliceFavoritesSystemComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SliceFavoritesSystemComponent, "{5580A7D0-CCD5-452C-A07B-7DD2C24B2A6E}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
 
        ////////////////////////////////////////////////////////////////////////
        //  SliceFavoritesRequestBus::Handler
        size_t GetNumFavorites() override;
        void EnumerateFavorites(const AZStd::function<void(const AZ::Data::AssetId& assetId)>& callback) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        //  SliceFavoritesSystemComponentRequestBus::Handler
        FavoriteDataModel* GetSliceFavoriteDataModel() override;
        ////////////////////////////////////////////////////////////////////////

    private:

        FavoriteDataModel* m_dataModel = nullptr;
    };

}
