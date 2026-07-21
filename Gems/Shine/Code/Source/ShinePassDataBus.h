/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AtomCore/Instance/Instance.h>
#include <Atom/RPI.Public/Base.h>

namespace AZ
{
    namespace RPI
    {
        class AttachmentImage;
        class RasterPass;
    }
}

namespace Shine
{
    using AttachmentImages = AZStd::vector<AZ::Data::Instance<AZ::RPI::AttachmentImage>>;
    using AttachmentImageAndDependentsPair = AZStd::pair<AZ::Data::Instance<AZ::RPI::AttachmentImage>, AttachmentImages>;
    using AttachmentImagesAndDependencies = AZStd::vector<AttachmentImageAndDependentsPair>;
}

class ShinePassRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = AZ::RPI::SceneId;

    //! Called when the number of render targets has changed and the Shine pass needs to rebuild
    virtual void RebuildRttChildren() = 0;

    //! Returns a render to texture pass based on render target name
    virtual AZ::RPI::RasterPass* GetRttPass(const AZStd::string& name) = 0;

    //! Returns the final pass that renders the UI canvas contents
    virtual AZ::RPI::RasterPass* GetUiCanvasPass() = 0;
};
using ShinePassRequestBus = AZ::EBus<ShinePassRequests>;

class ShinePassDataRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = AZ::RPI::SceneId;

    //! Get a list of render targets that require a render to texture pass, and any
    //! other render targets that are drawn on them
    virtual Shine::AttachmentImagesAndDependencies GetRenderTargets() = 0;
};
using ShinePassDataRequestBus = AZ::EBus<ShinePassDataRequests>;
