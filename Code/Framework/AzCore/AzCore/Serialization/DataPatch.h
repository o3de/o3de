/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_DATA_PATCH_FIELD_H
#define AZCORE_DATA_PATCH_FIELD_H

#include <AzCore/Asset/AssetCommon.h>

#include "ObjectStream.h"

namespace AZ
{
    struct Uuid;
    class ReflectContext;

    inline namespace DataPatchInternal
    {
        class AddressTypeSerializer;
        
        // Class to store information used in determining version, typeId and location in patch hierarchy for each class element examined between patch target (root) and patched element (leaf)
        class AddressTypeElement
        {
        public:
            AZ_TYPE_INFO(AddressTypeElement, "{6882E51A-6A3E-4CE8-AF80-0989F0AE61FF}")

            // Used for formatting the m_pathElement
            // Whether the element represents a class (Class), an element in a container (Index), unspecified (None)
            enum class ElementType
            {
                Class = 0,
                Index = 1,
                None = 2,
            };

            // Several other systems use the AddressType class for the addressElement
            // These default constructors allow reduced impact to those systems when we moved AddressType from u64 to AddressTypeElement
            AddressTypeElement(const AZ::u64 addressElement, const AZ::SerializeContext::ClassData* classData = nullptr, const AZ::SerializeContext::ClassElement* classElement = nullptr, ElementType elementType = ElementType::None);
            AddressTypeElement(const AZ::Crc32 addressElement, const AZ::SerializeContext::ClassData* classData = nullptr, const AZ::SerializeContext::ClassElement* classElement = nullptr, ElementType elementType = ElementType::None);

            // Constructors to build AddressTypeElements from another address type element and one changed aspect
            // Use only for class elements
            AddressTypeElement(const AZStd::string& newElementName, const AZ::u32 newElementVersion, const AddressTypeElement& oldElement);
            AddressTypeElement(const AZ::TypeId& newTypeId, const AZ::u32 newElementVersion, const AddressTypeElement& original);

            const AZ::TypeId& GetElementTypeId() const { return m_addressClassTypeId; }
            const AZStd::string& GetPathElement() const { return m_pathElement; }
            const AZ::u64 GetAddressElement() const { return m_addressElement; }
            const AZ::u32 GetElementVersion() const { return m_addressClassVersion; }
            bool IsValid() const { return m_isValid; }

            static constexpr const char* PathDelimiter = "/";
            static constexpr const char* VersionDelimiter = u8"\u00B7"; // utf-8 for <middledot>

            void SetAddressClassTypeId(const AZ::TypeId& id) { m_addressClassTypeId = id; }
            void SetPathElement(const AZStd::string& path) { m_pathElement = path; }
            void SetAddressElement(const u64 address) { m_addressElement = address; }
            void SetAddressClassVersion(const AZ::u32 version) { m_addressClassVersion = version; }

            friend bool operator==(const AddressTypeElement& lhs, const AddressTypeElement& rhs);
            friend bool operator!=(const AddressTypeElement& lhs, const AddressTypeElement& rhs);

        private:
            friend class AddressTypeSerializer;

            AddressTypeElement();               // Default constructor to supply an empty Element to AddressTypeSerializer to fill out
            AZ::TypeId m_addressClassTypeId;    // TypeId of the class element at the time it was stored
            AZStd::string m_pathElement;        // Path representation of element for building full path string
            AZ::u64 m_addressElement;           // Used to identify a reflected class element on the path from patch target to patched element
            AZ::u32 m_addressClassVersion;      // Version of the class element at the time it was stored
            bool m_isValid = true;              // Always true unless deserialization of pathElement fails
        };

        struct AddressType
            : public AZStd::vector<AddressTypeElement>
        {
        public:
            AZ_TYPE_INFO(AddressType, "{90752F2D-CBD3-4EE9-9CDD-447E797C8408}");

            bool IsValid() const { return m_isValid; }
            bool IsFromLegacyAddress()
            {
                for (const auto& element : *this)
                {
                    if (!element.GetElementTypeId().IsNull())
                    {
                        return false;
                    }
                }

                return true;
            }

        private:
            friend class AddressTypeSerializer;
            bool m_isValid = true;          // Always true unless one of its AddressTypeElements is not valid
            bool m_isLegacy = false;
        };

        using PatchMap = AZStd::unordered_map<AddressType, AZStd::any>;
        using ChildPatchMap = AZStd::unordered_map<AddressType, AZStd::vector<AddressType> >;

        template<typename Void, typename...>
        struct has_common_type_helper
            : AZStd::false_type
        {};

        template<typename... Types>
        struct has_common_type_helper<AZStd::void_t<std::common_type_t<Types...>>, Types...>
            : AZStd::true_type
        {};

        template<typename... Types>
        using has_common_type = has_common_type_helper<void, Types...>;

        /**
        * Custom serializer for our address type, as we want to be more space efficient and not store every element of the container
        * separately.
        */
        class AddressTypeSerializer
            : public AZ::Internal::AZBinaryData
        {
        public:

            AddressTypeElement LoadAddressElementFromPath(const AZStd::string& pathElement) const;

        private:
            bool Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian = false) override;

            void LoadElementFinalize(AddressTypeElement& addressElement, const AZStd::string& pathElement, const AZStd::string& version) const;
            void LoadLegacyElement(AddressTypeElement& addressElement, const AZStd::string& pathElement, const size_t pathDelimLength) const;

            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) override;

            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian) override;

            size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override;

            bool CompareValueData(const void* lhs, const void* rhs) override;
        };
    }
}

//! AZ namespace needs to be closed in order to specialize the AZStd::hash struct for AddressTypeElement and AddressType
namespace AZStd
{
    template<>
    struct hash<AZ::DataPatchInternal::AddressTypeElement>
    {
        // For hashing
        AZStd::size_t operator()(const AZ::DataPatchInternal::AddressTypeElement& key) const
        {
            // Only hash the address element so version doesn't affect if we can find patch data
            return AZStd::hash<AZ::u64>()(key.GetAddressElement());
        }
    };
    /// used for hashing
    template<>
    struct hash<AZ::DataPatchInternal::AddressType>
    {
        size_t operator()(const AZ::DataPatchInternal::AddressType& key) const
        {
            return AZStd::hash_range(key.begin(), key.end());
        }
    };
}


//! Reopen namespace to define DataPatch class
namespace AZ
{
    /**
    * Structure that contains patch data for a given class. The primary goal of this
    * object is to help with tools (slices and undo/redo), this structure is not recommended to
    * runtime due to efficiency. If you want dynamically configure objects, use data overlay which are more efficient and generic.
    */
    class DataPatch
    {
    public:

        /**
        * Data Patches have been updated to be human readable.
        * For backwards compatibility the DataPatch type Uuid has been modified and a deprecation converter is used to convert.
        */
        AZ_CLASS_ALLOCATOR(DataPatch, SystemAllocator);
        AZ_TYPE_INFO(DataPatch, "{BFF7A3F5-9014-4000-92C7-9B2BC7913DA9}")

        /**
         * Data addresses can be tagged with flags to affect how patches are created and applied.
         */
        using Flags = AZ::u8;

        /**
         * Bit fields for data flags.
         * These options affect how patches are created and applied.
         * There are two categories of flags:
         * 1. Set: These flags are manually set by users at a specific address. They are serialized to disk.
         * 2. Effect: These flags denote the addresses influenced by a "Set" flag. They are runtime-only and should not be serialized to disk.
         */
        enum Flag : Flags
        {
            // "Set" flags are serialized to disk and should never have their values changed.

            ForceOverrideSet = 1 << 0, ///< Data at this address always overrides data from the source.
            PreventOverrideSet = 1 << 1, ///< Data at this address can't be overridden by a target.
            HidePropertySet = 1 << 2, ///< The property associated with this address will be hidden when viewing a target.

            // "Effect" flags occupy the upper half of bits available in a DataPatch::Flags variable.
            // If we run out of space to add more flags, increase the size of DataPatch::Flags
            // and change the values of the "effect" flags.

            ForceOverrideEffect = 1 << 4, ///< Data at this address is affected by ForceOverride and always overrides its source.
            PreventOverrideEffect = 1 << 5, ///< Data at this address is affected by PreventOverride and cannot override its source.
            HidePropertyEffect = 1 << 6, ///< Data at this address is affected by HideProperty and cannot be seen when viewing the target.

            // Masks
            SetMask = 0x0F,
            EffectMask = 0xF0,
        };

        // Helper class to capture bytestreams from Legacy DataPatches of typeId {3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03} during conversion
        struct LegacyStreamWrapper
        {
            AZ_TYPE_INFO(LegacyStreamWrapper, "{BC53E2ED-2C28-4B27-99AA-098CFC7F0C66}");
            AZStd::vector<AZ::u8> m_stream;
        };

        //! Alias the DataPatchInternal::AddressType inside the DataPatch declaration for backwards compatibility with DataPatch::AddressType
        using AddressTypeElement = DataPatchInternal::AddressTypeElement;
        using AddressType = DataPatchInternal::AddressType;
        using PatchMap = DataPatchInternal::PatchMap;
        using ChildPatchMap = DataPatchInternal::ChildPatchMap;
        /**
         * Data flags, mapped by the address where the flags are set.
         * When patching, an address is affected not only by the flags set AT that address,
         * but also the flags set at any parent address.
         */
        using FlagsMap = AZStd::unordered_map<AddressType, Flags>;

        /// Given data flags affecting the parent address, return the effect upon the child address.
        /// An example of a parent and child would be a vector and an element in that vector.
        static Flags GetEffectOfParentFlagsOnThisAddress(Flags flagsAtParentAddress);

        /// Given data flags for source data at this address, return the effect on a DataPatch.
        /// An example of source data would be the slice upon which a slice-instance is based.
        static Flags GetEffectOfSourceFlagsOnThisAddress(Flags flagsAtSourceAddress);

        /// Given data flags for target data at this address, return the effect on a DataPatch.
        static Flags GetEffectOfTargetFlagsOnThisAddress(Flags flagsAtTargetAddress);

        DataPatch();
        DataPatch(const DataPatch& rhs);
        DataPatch(DataPatch&& rhs);
        DataPatch& operator=(DataPatch&& rhs);
        DataPatch& operator=(const DataPatch& rhs);

        /**
         * Create a patch structure which generate the delta (patcher) from source to the target type.
         *
         * \param source object we will use a base for the delta
         * \param souceClassId class of the source type, either the same as targetClassId or a base class
         * \param target object we want to have is we have source and apply the returned patcher.
         * \param targetClassId class of the target type, either the same as sourceClassId or a base class
         * \param sourceFlagsMap (optional) flags for source data. These may affect how the patch is created (ex: prevent patches at specific addresses)
         * \param targetFlagsMap (optional) flags for target data. These may affect how the patch is created (ex: force patches at specific addresses) 
         * \param context if null we will grab the default serialize context.
         */
        bool Create(
            const void* source, 
            const Uuid& souceClassId, 
            const void* target, 
            const Uuid& targetClassId, 
            const FlagsMap& sourceFlagsMap = FlagsMap(), 
            const FlagsMap& targetFlagsMap = FlagsMap(), 
            SerializeContext* context = nullptr);

        /// T and U should either be the same type a common base class
        template<class T, class U>
        bool Create(
            const T* source, 
            const U* target, 
            const FlagsMap& sourceFlagsMap = FlagsMap(), 
            const FlagsMap& targetFlagsMap = FlagsMap(), 
            SerializeContext* context = nullptr)
        {
            const void* sourceClassPtr = SerializeTypeInfo<T>::RttiCast(source, SerializeTypeInfo<T>::GetRttiTypeId(source));
            const Uuid& sourceClassId = SerializeTypeInfo<T>::GetUuid(source);
            const void* targetClassPtr = SerializeTypeInfo<U>::RttiCast(target, SerializeTypeInfo<U>::GetRttiTypeId(target));
            const Uuid& targetClassId = SerializeTypeInfo<U>::GetUuid(target);
            return Create(sourceClassPtr, sourceClassId, targetClassPtr, targetClassId, sourceFlagsMap, targetFlagsMap, context);
        }

        /**
         * Apply the patch to a source instance and generate a patched instance, from a source instance.
         * If patch can't be applied a null pointer is returned. Currently the only reason for that is if
         * the resulting class ID doesn't match the stored root of the patch.
         *
         * \param source pointer to the source instance.
         * \param sourceClassID id of the class \ref source is pointing to.
         * \param context if null we will grab the default serialize context.
         * \param filterDesc customizable filter for data in patch (ex: filter out classes and assets)
         * \param sourceFlagsMap flags for source data. These may affect how a patch is applied (ex: prevent patching of specific addresses)
         * \param targetFlagsMap flags for target data. These may affect how a patch is applied.
         */
        void* Apply(
            const void* source, 
            const Uuid& sourceClassId, 
            SerializeContext* context = nullptr, 
            const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor(), 
            const FlagsMap& sourceFlagsMap = FlagsMap(), 
            const FlagsMap& targetFlagsMap = FlagsMap()) const;

        // Apply specialization when the source and return type are the same.
        template<class T>
        T* Apply(
            const T* source, 
            SerializeContext* context = nullptr, 
            const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor(), 
            const FlagsMap& sourceFlagsMap = FlagsMap(), 
            const FlagsMap& targetFlagsMap = FlagsMap()) const
        {
            const void* classPtr = SerializeTypeInfo<T>::RttiCast(source, SerializeTypeInfo<T>::GetRttiTypeId(source));
            const Uuid& classId = SerializeTypeInfo<T>::GetUuid(source);
            if (m_targetClassId == classId)
            {
                return reinterpret_cast<T*>(Apply(classPtr, classId, context, filterDesc, sourceFlagsMap, targetFlagsMap));
            }
            else
            {
                return nullptr;
            }
        }

        template<class U, class T>
        U* Apply(
            const T* source, 
            SerializeContext* context = nullptr, 
            const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor(), 
            const FlagsMap& sourceFlagsMap = FlagsMap(), 
            const FlagsMap& targetFlagsMap = FlagsMap()) const
        {
            // \note find class Data and check if we can do that cast ie. U* is a base class of m_targetClassID
            if (SerializeTypeInfo<U>::GetUuid() == m_targetClassId)
            {
                return reinterpret_cast<U*>(Apply(source, SerializeTypeInfo<T>::GetUuid(), context, filterDesc, sourceFlagsMap, targetFlagsMap));
            }
            else
            {
                return nullptr;
            }
        }

        /// \returns true if this is a valid patch.
        bool IsValid() const
        {
            return m_targetClassId != Uuid::CreateNull();
        }

        /// \returns true of the patch actually contains data for patching otherwise false.
        bool IsData() const
        {
            return !m_patch.empty();
        }

        /**
         * Reflect a patch for serialization.
         */
        static void Reflect(ReflectContext* context);

        AZ_INLINE static const AZ::Uuid GetLegacyDataPatchTypeId()
        {
            return Uuid("{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}");
        }

    protected:
        Uuid     m_targetClassId;
        unsigned int m_targetClassVersion;
        mutable PatchMap m_patch;
    };

    /**
    * Structure used to pass information about the data patch being applied into the event handler's OnPatchBegin/OnPatchEnd
    * methods. Using this structure allows it to be forward declared in SerializeContext.h thus avoiding circular header file 
    * dependencies.
    */
    struct DataPatchNodeInfo
    {
        const AddressType& address;
        const PatchMap& patch;
        const ChildPatchMap& childPatchLookup;
    };

}   // namespace AZ

#endif
