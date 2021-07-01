/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace MaterialEditor
{
    //! MaterialEditorWindowFactoryRequestBus provides 
    class MaterialEditorWindowFactoryRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        /// Creates and shows the MaterialEditorWindow
        virtual void CreateMaterialEditorWindow() = 0;

        //! Destroys material editor window and releases all cached assets
        virtual void DestroyMaterialEditorWindow() = 0;
    };
    using MaterialEditorWindowFactoryRequestBus = AZ::EBus<MaterialEditorWindowFactoryRequests>;

} // namespace MaterialEditor
