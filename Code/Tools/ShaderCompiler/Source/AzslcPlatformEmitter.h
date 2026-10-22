/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <string>

#include "AzslcBackend.h"

namespace AZ::ShaderCompiler
{
    struct CodeEmitter;

    //! Modes of subpass input the emitter supports
    enum class SubpassInputSupportFlag : uint32_t
    {
        None = 0,                   // No support
        Color = 1 << 0,             // Support for color attachments
        DepthStencil = 1 << 1,      // Support for depth/stencil attachment
        All = Color | DepthStencil  // Support all modes
    };

    // PlatformEmitter is not a Backend by design. It's a supplement to CodeEmitter, not a replacement.
    struct PlatformEmitter 
    {
        using SrgParamDesc = RootSigDesc::SrgParamDesc;

        //! Returns the default platform emitter. It's always guaranteed to exist.
        static const PlatformEmitter* GetDefaultEmitter() noexcept(true);

        //! Returns a platform emitter registered with the specified key.
        //! If no such key is registered, returns nullptr instead.
        //! It never returns the default platform emitter.
        //! @param key  The key used to search the platform emitter
        static const PlatformEmitter* GetEmitter(const string& key) noexcept(true);

        PlatformEmitter(PlatformEmitter const&) = delete;
        void operator=(PlatformEmitter const&)  = delete;
        PlatformEmitter(PlatformEmitter&&) = delete;
        PlatformEmitter& operator=(PlatformEmitter&&) = delete;

    protected:
        PlatformEmitter() {};
        virtual ~PlatformEmitter() {}

        //! Registers a new platform emitter with the specified name.
        //! In order to provide robust assertion it throws an exception if more than one emitters try to use the same key.
        //! The only intended use of this method is by the platform emitter itself. No other entity should register emitters.
        //! @param key  The key used to search. The platform emitter will be registered under this key.
        //! @param platformEmitter  An emitter to register, which must be of a class derived from this PlatformEmitter
        static void SetEmitter(const string& key, const PlatformEmitter* const platformEmitter) noexcept(false);

    public:
        //! Gets the string emission for the root signature for this platform 
        //! @param codeEmitter  Reference to the calling code emitter
        //! @param rootSig      Root signature description which should be emitted as shader code
        //! @param options      Emission options
        [[nodiscard]]
        virtual string GetRootSig(const CodeEmitter& codeEmitter, const RootSigDesc& rootSig, const Options& options, BindingPair::Set signatureQuery) const;

        //! Gets the string emission for the data view containing the root constants
        //! @param codeEmitter  Reference to the calling code emitter
        //! @param rootSig      Root signature description which should be emitted as shader code
        //! @param options      Emission options
        [[nodiscard]]
        virtual string GetRootConstantsView(const CodeEmitter& codeEmitter, const RootSigDesc& rootSig, const Options& options, BindingPair::Set signatureQuery) const;

        //! Gets the string emission for the surroundings of an extern data view variable
        //! @param codeEmitter              Reference to the calling code emitter
        //! @param symbol                   The symbol path of the original variable we are emitting as an extern data view
        //! @param bindInfoRegisterIndex    Register index of the resource
        //! @param stringifiedLogicalSpace  Optional register space
        //! @param options                  Emission options
        //! \return                         first: header to emit before the dataview declaration.   second: footer to emit after the dataview declaration
        [[nodiscard]]
        virtual std::pair<string, string> GetDataViewHeaderFooter(
            const CodeEmitter& codeEmitter,
            const IdentifierUID& symbol, 
            uint32_t bindInfoRegisterIndex,
            string_view registerTypeLetter,
            optional<string> stringifiedLogicalSpace,
            const Options& options) const;

        //! Aligns the size for the data containing the root constants.
        //! @param size  The size of stride
        virtual uint32_t AlignRootConstants(uint32_t size) const;

        virtual bool RequiresUniqueSpaceForUnboundedArrays() const {return false;}

        [[nodiscard]]
        virtual string GetSpecializationConstant(const CodeEmitter& codeEmitter, const IdentifierUID& symbol, const Options& options) const;

        //! Returns the subpass input that the platform emitter supports.
        virtual SubpassInputSupportFlag GetSubpassInputSupport() const;
    };
}
