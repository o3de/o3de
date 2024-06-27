/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ::RHI
{
    class Scope;
    class DeviceShaderResourceGroup;
    struct ShaderResourceGroupBindingInfo;
    class Resource;
    class DeviceResourceView;
    class ScopeAttachment;
    class FrameAttachment;

    //! This is a utility for validating that resources are in a correct state to be used for
    //! graphics / compute / copy operations on a command list. It does by crawling ShaderResourceGroups
    //! and checking that the Scope has properly declared the relevant pools and attachments.
    class CommandListValidator
    {
    public:
        CommandListValidator() = default;

        //! Begins validation of the provided scope. All validation calls must
        //! remain within a BeginScope EndScope block.
        void BeginScope(const Scope& scope);

        //! Validates that the shader resource group is usable on the current scope. Emits a warning
        //! otherwise and returns false.
        bool ValidateShaderResourceGroup(const DeviceShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupBindingInfo& bindingInfo) const;

        //! Ends validation for the current scope.
        void EndScope();

    private:
        struct ValidateViewContext
        {
            const char* m_scopeName = "";
            const char* m_srgName = "";
            const char* m_shaderInputTypeName = "";
            ScopeAttachmentAccess m_scopeAttachmentAccess = ScopeAttachmentAccess::Read;
            const DeviceResourceView* m_resourceView = nullptr;
        };

        static ScopeAttachmentAccess GetAttachmentAccess(ShaderInputBufferAccess bufferInputAccess);
        static ScopeAttachmentAccess GetAttachmentAccess(ShaderInputImageAccess imageInputAccess);

        bool ValidateView(const ValidateViewContext& context, bool ignoreAttachmentValidation) const;
        bool ValidateAttachment(const ValidateViewContext& context, const FrameAttachment* frameAttachment) const;
            
        AZStd::unordered_map<const Resource*, AZStd::vector<const ScopeAttachment*>> m_attachments;
        const Scope* m_scope = nullptr;
    };
}
