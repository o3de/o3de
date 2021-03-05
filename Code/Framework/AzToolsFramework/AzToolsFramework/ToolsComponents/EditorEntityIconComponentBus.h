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

#include <AzCore/Component/ComponentBus.h>

#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AzToolsFramework
{
    /**
    * EBus calls for setting / getting information about the entity icon.
    */
    class EditorEntityIconComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Set entity icon by assigning an Entity Icon Asset.
         */
        virtual void SetEntityIconAsset(const AZ::Data::AssetId& assetId) = 0;

        /**
         * Get the entity icon asset id.
         * If the returned asset id is invalid, an icon of one of the entity's components will be used instead.
         */
        virtual AZ::Data::AssetId GetEntityIconAssetId() = 0;

        /**
         * Get the full path of the source icon image associated with the current entity.
         */
        virtual AZStd::string GetEntityIconPath() = 0;

        /**
         * Get the texture id of this entity icon.
         * The Id is used to lookup the texture in the graphics system.
         */
        virtual int GetEntityIconTextureId() = 0;

        /**
         * Get the hide flag for the entity icon in viewport.
         * @return A boolean denoting whether the entity icon should be hidden in viewport.
         */
        virtual bool IsEntityIconHiddenInViewport() = 0;
    };

    using EditorEntityIconComponentRequestBus = AZ::EBus<EditorEntityIconComponentRequests>;

    /**
    * EBus events about entity icon.
    */
    class EditorEntityIconComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        /**
        * EBus events fired when an entity's icon changed.
        */
        virtual void OnEntityIconChanged(const AZ::Data::AssetId& entityIconAssetId) = 0;
    };

    using EditorEntityIconComponentNotificationBus = AZ::EBus<EditorEntityIconComponentNotifications>;
}