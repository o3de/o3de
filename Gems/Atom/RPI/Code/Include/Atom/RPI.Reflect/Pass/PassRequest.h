/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RPI.Reflect/Pass/PassAttachmentReflect.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>

#include <AzCore/std/containers/span.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RPI
    {
        using SlotIndex = RHI::Handle<uint32_t>;

        //! This class represents a request for a Pass to be instantiated from a PassTemplate
        //! It also contains a list of inputs for the instantiated pass
        struct ATOM_RPI_REFLECT_API PassRequest final
        {
            AZ_TYPE_INFO(PassRequest, "{C43802D1-8501-4D7A-B642-85F8646DF46D}");
            AZ_CLASS_ALLOCATOR(PassRequest, SystemAllocator);
            static void Reflect(ReflectContext* context);

            PassRequest() = default;
            ~PassRequest() = default;

            //! Add a pass connection to the list of input connections
            void AddInputConnection(PassConnection inputConnection);

            //! Name of the pass this request will instantiate
            Name m_passName;

            //! Name of the template from which the pass will be created
            Name m_templateName;

            //! Names of Passes that this Pass should execute after
            AZStd::vector<Name> m_executeAfterPasses;

            //! Names of Passes that this Pass should execute before
            AZStd::vector<Name> m_executeBeforePasses;

            //! Connections for the instantiated Pass
            //! Most of the time these will be input connections that point to outputs of other passes
            //! Cases where you would want to specify output connections are to connect to image or
            //! buffer attachment overrides in the lists below
            PassConnectionList m_connections;

            //! List of descriptors for the image attachments the PassRequest will create
            //! If the pass template already specifies an attachment with the same name,
            //! the PassRequest will override that attachment
            PassImageAttachmentDescList m_imageAttachmentOverrides;

            //! List of descriptors for the buffer attachments the PassRequest will create
            //! If the pass template already specifies an attachment with the same name,
            //! the PassRequest will override that attachment
            PassBufferAttachmentDescList m_bufferAttachmentOverrides;

            //! Optional data to be used during pass initialization
            AZStd::shared_ptr<PassData> m_passData = nullptr;

            //! Initial state of the pass when created (enabled/disabled)
            bool m_passEnabled = true;
        };

        using PassRequestList = AZStd::vector<PassRequest>;
        using PassRequestListView = AZStd::span<const PassRequest>;

    }   // namespace RPI
}   // namespace AZ
