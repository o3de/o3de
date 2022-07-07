/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Data
    {
        template<class T>
        class Asset;
    }

    namespace Render
    {
        using ModelReloadedEvent = Event<const Data::Asset<RPI::ModelAsset>>;

        //! A system that handles reloading the hierarchy of model assets in the correct order
        class ModelReloaderSystemInterface
        {
        public:
            AZ_RTTI(AZ::Render::ModelReloaderSystemInterface, "{E7E05B1F-8928-4A1B-B75D-3D5433E65BCA}");

            ModelReloaderSystemInterface()
            {
                Interface<ModelReloaderSystemInterface>::Register(this);
            }

            virtual ~ModelReloaderSystemInterface()
            {
                Interface<ModelReloaderSystemInterface>::Unregister(this);
            }

            static ModelReloaderSystemInterface* Get()
            {
                return Interface<ModelReloaderSystemInterface>::Get();
            }

            //! Requests a model reload and passes in a callback event handler for when the reload is finished
            virtual void ReloadModel(
                Data::Asset<RPI::ModelAsset> modelAsset, ModelReloadedEvent::Handler& onReloadedEventHandler) = 0;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(ModelReloaderSystemInterface);
        };
    } // namespace Render
} // namespace AZ
