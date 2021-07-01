/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! Manages system-wide initialization and support for Model classes
        class ModelSystem
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            void Init();
            void Shutdown();
        };
    } // namespace RPI
} // namespace AZ
