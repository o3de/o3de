/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! Manages system-wide initialization and support for Model classes
        class ATOM_RPI_PUBLIC_API ModelSystem
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            void Init();
            void Shutdown();
        };
    } // namespace RPI
} // namespace AZ
