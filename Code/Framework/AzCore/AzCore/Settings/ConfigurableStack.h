/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

namespace AZ
{
    //! The ConfigurableStack makes configuring stacks and arrays through the Settings Registry easier than using direct serialization
    //! to a container like AZStd::vector. It does this by using JSON Objects rather than JSON arrays, although arrays are supported
    //! for backwards compatibility. Two key words were added:
    //!     - $stack_before : Insert the new entry before the referenced entry. Referencing is done by name.
    //!     - $stack_after  : Insert the new entry after the referenced entry. Referencing is done by name.
    //! to allow inserting new entries at specific locations. An example of a .setreg file at updates existing settings would be:
    //! // Original settings
    //! {
    //!     "Settings in a stack":
    //!     {
    //!         "AnOriginalEntry":
    //!         {
    //!             "MyValue": "hello",
    //!             "ExampleValue": 84
    //!         },
    //!         "TheSecondEntry":
    //!         {
    //!             "MyValue": "world"
    //!         }
    //!     }
    //! }
    //! 
    //! // Customized settings.
    //! {
    //!     "Settings in a stack":
    //!     {
    //!         // Add a new entry before "AnOriginalEntry" in the original document.
    //!         "NewEntry":
    //!         {
    //!             "$stack_before": "AnOriginalEntry",
    //!             "MyValue": 42
    //!         },
    //!         // Add a second entry after "AnOriginalEntry" in the original document.
    //!         "SecondNewEntry":
    //!         {
    //!             "$stack_after": "AnOriginalEntry",
    //!             "MyValue": "FortyTwo".
    //!         },
    //!         // Update a value in "AnOriginalEntry".
    //!         "AnOriginalEntry":
    //!         {
    //!             "ExampleValue": 42
    //!         },
    //!         // Delete the "TheSecondEntry" from the settings.
    //!         "TheSecondEntry" : null, 
    //!     }
    //! }
    //!
    //! The ConfigurableStack uses an AZStd::shared_ptr to store the values. This supports settings up a base class and specifying
    //! derived classes in the settings, but requires that the base and derived classes all have a memory allocator associated with
    //! them (i.e. by using the "AZ_CLASS_ALLOCATOR" macro) and that the relation of the classes is reflected. Loading a
    //! ConfigurableStack can be done using the GetObject call on the SettingsRegistryInterface.

    class ReflectContext;

    class ConfigurableStackInterface
    {
    public:
        friend class JsonConfigurableStackSerializer;

        virtual ~ConfigurableStackInterface() = default;

        virtual TypeId GetNodeType() const = 0;

    protected:
        enum class InsertPosition
        {
            Before,
            After
        };
        virtual void* AddNode(AZStd::string name) = 0;
        virtual void* AddNode(AZStd::string name, AZStd::string_view target, InsertPosition position) = 0;
    };

    template<typename StackBaseType>
    class ConfigurableStack final : public ConfigurableStackInterface
    {
    public:
        using NodeValue = AZStd::shared_ptr<StackBaseType>;
        using Node = AZStd::pair<AZStd::string, NodeValue>;
        using NodeContainer = AZStd::vector<Node>;
        using Iterator = typename NodeContainer::iterator;
        using ConstIterator = typename NodeContainer::const_iterator;

        ~ConfigurableStack() override = default;

        TypeId GetNodeType() const override;

        Iterator begin();
        Iterator end();
        ConstIterator begin() const;
        ConstIterator end() const;
        
        bool empty() const;
        size_t size() const;
        void clear();

    protected:
        void* AddNode(AZStd::string name) override;
        void* AddNode(AZStd::string name, AZStd::string_view target, InsertPosition position) override;

    private:
        NodeContainer m_nodes;
    };

    AZ_TYPE_INFO_TEMPLATE(ConfigurableStack, "{0A3D2038-6E6A-4EFD-A1B4-F30D947E21B1}", AZ_TYPE_INFO_TYPENAME);

    template<typename StackBaseType>
    struct SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>
    {
        using ConfigurableStackType = ConfigurableStack<StackBaseType>;

        class GenericConfigurableStackInfo : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericConfigurableStackInfo, "{FC5A9353-D0DE-48F6-81B5-1CB2985C5F65}");

            GenericConfigurableStackInfo();

            SerializeContext::ClassData* GetClassData() override;
            size_t GetNumTemplatedArguments() override;
            AZ::TypeId GetTemplatedTypeId([[maybe_unused]] size_t element) override;
            AZ::TypeId GetSpecializedTypeId() const override;
            AZ::TypeId GetGenericTypeId() const override;
            
            void Reflect(SerializeContext* serializeContext) override;

            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericConfigurableStackInfo;

        static ClassInfoType* GetGenericInfo();
        static AZ::TypeId GetClassTypeId();
    };

    class JsonConfigurableStackSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonConfigurableStackSerializer, "{45A31805-9058-41A9-B1A3-71E2CB4D9237}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(
            void* outputValue,
            const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            const Uuid& valueTypeId,
            JsonSerializerContext& context) override;

        static void Reflect(ReflectContext* context);

    private:
        JsonSerializationResult::Result LoadFromArray(
            void* outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);
        JsonSerializationResult::Result LoadFromObject(
            void* outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);

        static constexpr const char* StackBefore = "$stack_before";
        static constexpr const char* StackAfter = "$stack_after";
    };
} // namespace AZ

#include <AzCore/Settings/ConfigurableStack.inl>
