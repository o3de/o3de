/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    class ShaderResourceGroupLayout;

    //! Utility class to manage looking up indices via Names
    //! Users can initialize this class with the Name used to lookup the index and
    //! then use this class as an index. Under the hood, the code will initialize
    //! the index if it's not already initialized before using it.
    struct ShaderInputNameIndex
    {
        AZ_TYPE_INFO(ShaderInputNameIndex, "{1A9A92A7-9289-45E1-9EFE-D08257EF2BF1}");
        static void Reflect(AZ::ReflectContext* context);

        ShaderInputNameIndex() = default;
        ShaderInputNameIndex(Name name);
        ShaderInputNameIndex(const char* name);

        void operator =(Name name);
        void operator =(const char* name);

        // Functions for looking up the index of the given name...
        void FindBufferIndex(const ShaderResourceGroupLayout* srgLayout);
        void FindImageIndex(const ShaderResourceGroupLayout* srgLayout);
        void FindSamplerIndex(const ShaderResourceGroupLayout* srgLayout);
        void FindConstantIndex(const ShaderResourceGroupLayout* srgLayout);

        // Functions that will validate the index and try to find it if it is not initialized...
        bool ValidateOrFindBufferIndex(const ShaderResourceGroupLayout* srgLayout);
        bool ValidateOrFindImageIndex(const ShaderResourceGroupLayout* srgLayout);
        bool ValidateOrFindSamplerIndex(const ShaderResourceGroupLayout* srgLayout);
        bool ValidateOrFindConstantIndex(const ShaderResourceGroupLayout* srgLayout);

        // Functions for retrieving the index...
        ShaderInputBufferIndex GetBufferIndex() const;
        ShaderInputImageIndex GetImageIndex() const;
        ShaderInputConstantIndex GetConstantIndex() const;
        ShaderInputSamplerIndex GetSamplerIndex() const;
        ShaderInputStaticSamplerIndex GetStaticSamplerIndex() const;

        // Invalidates all members except the name
        // Call this if you want to keep the name but want the index to be recalculated on next use
        void Reset();

        // Checks and asserts...
        bool HasName() const;
        void AssertHasName() const;
        bool IsValid() const;
        void AssertValid() const;
        bool IsInitialized() const;
        void AssetInialized() const;

        // Retrieves the underlying name. Should only be used for debug purposes like printing the name when we fail to bind to the SRG.
        // All regular functionality should go through the above functions.
        const Name& GetNameForDebug() const;

    private:

        enum class IndexType : u32
        {
            // Shader input indices
            ShaderBuffer = 0,
            ShaderImage,
            ShaderSampler,
            ShaderConstant,

            Count,
            InvalidIndex = Count
        };

        // Asserts name, assigns input type and sets initialization flag to true 
        void Initialize(IndexType indexType);

        // Asserts index is valid and returns it
        u32 GetIndex() const;

        // Template getter with static_cast<> for improved readability in other functions
        template<typename T>
        T GetIndexAs() const;

        // The Name used to lookup the index
        Name m_name;

        // The cached index 
        Handle<> m_index;

        struct
        {
            // Whether the class has already used the name to lookup the index
            // This avoids repeated lookups if the lookup returns an invalid index
            u32 m_initialized : 1;

            // The index type, used for validation and error catching
            IndexType m_inputType : 8;
        };
    };
}
