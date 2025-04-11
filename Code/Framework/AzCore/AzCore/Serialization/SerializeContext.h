/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <limits>

#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/Serialization/SerializeContext_fwd.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_abstract.h>
#include <AzCore/std/typetraits/negation.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/any.h>
#include <AzCore/std/parallel/atomic.h>

#include <AzCore/std/functional.h>

#include <AzCore/RTTI/ReflectContext.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/IO/ByteContainerStream.h>

#define AZ_SERIALIZE_BINARY_STACK_BUFFER 4096

#define AZ_SERIALIZE_SWAP_ENDIAN(_value, _isSwap)  if (_isSwap) AZStd::endian_swap(_value)

namespace AZ::Data
{
    template<typename T>
    class Asset;
}

namespace AZ
{
    class EditContext;

    class ObjectStream;
    class GenericClassInfo;
    struct DataPatchNodeInfo;
    class DataOverlayTarget;
}

 namespace AZ::ObjectStreamInternal
 {
    class ObjectStreamImpl;
 }

namespace AZ::SerializeContextAttributes
{
    // Attribute used to set an override function on a SerializeContext::ClassData attribute array
    // which can be used to override the ObjectStream WriteElement call to write out reflected data differently
    static const AZ::Crc32 ObjectStreamWriteElementOverride = AZ_CRC_CE("ObjectStreamWriteElementOverride");
}
namespace AZ
{
    enum class ObjectStreamWriteOverrideResponse
    {
        CompletedWrite,
        FallbackToDefaultWrite,
        AbortWrite
    };
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(ObjectStreamWriteOverrideResponse);
}
namespace AZ::Edit
{
    struct ElementData;
    struct ClassData;
}

namespace AZ::Serialize
{
    /**
    * Template to hold a single global instance of a particular type. Keep in mind that this is not DLL safe
    * get the variable from the Environment if store globals with state, currently this is not the case.
    * \ref AllocatorInstance for example with environment as we share allocators state across DLLs
    */
    template<class T>
    struct StaticInstance
    {
        static T s_instance;
    };
    template<class T>
    T StaticInstance<T>::s_instance;

    namespace Attributes
    {
        extern const Crc32 EnumValueKey;
        extern const Crc32 EnumUnderlyingType;
    }

    inline constexpr unsigned int VersionClassDeprecated = (unsigned int)-1;
    using IDataSerializerDeleter = AZStd::function<void(IDataSerializer* ptr)>;
    using IDataSerializerPtr = AZStd::unique_ptr<IDataSerializer, IDataSerializerDeleter>;
}

namespace AZ
{
    using AttributePtr = AZStd::shared_ptr<Attribute>;
    using AttributeSharedPair = AZStd::pair<AttributeId, AttributePtr>;
    template <typename T, typename ContainerType = AttributeContainerType<T>>
    AttributePtr CreateModuleAttribute(T&& attrValue);

    /**
     * Serialize context is a class that manages information
     * about all reflected data structures. You will use it
     * for all related information when you declare you data
     * for serialization. In addition it will handle data version control.
     */
    class SerializeContext
        : public ReflectContext
    {
    public:
        /// @cond EXCLUDE_DOCS
        friend class EditContext;
        class ClassBuilder;
        class EnumBuilder;
        /// @endcond

        using ClassData = Serialize::ClassData;
        using EnumerateInstanceCallContext = Serialize::EnumerateInstanceCallContext;
        using ClassElement = Serialize::ClassElement;
        using DataElement = Serialize::DataElement;
        using DataElementNode = Serialize::DataElementNode;
        using IObjectFactory = Serialize::IObjectFactory;
        using IDataSerializer = Serialize::IDataSerializer;
        using IDataContainer = Serialize::IDataContainer;
        using IEventHandler = Serialize::IEventHandler;
        using IDataConverter = Serialize::IDataConverter;

        AZ_CLASS_ALLOCATOR(SerializeContext, SystemAllocator);
        AZ_TYPE_INFO_WITH_NAME_DECL(SerializeContext);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        /// Callback to process data conversion.
        using VersionConverter =  bool(*)(SerializeContext& /*context*/, DataElementNode& /* elements to convert */);

        /// Callback for persistent ID for an instance \note: We should probably switch this to UUID
        using ClassPersistentId = u64(*)(const void* /*class instance*/);

        /// Callback to manipulate entity saving on a yes/no base, otherwise you will need provide serializer for more advanced logic.
        using ClassDoSave = bool(*)(const void* /*class instance*/);

        // \todo bind allocator to serialize allocator
        using UuidToClassMap = AZStd::unordered_map<Uuid, ClassData>;

        /// If registerIntegralTypes is true we will register the default serializer for all integral types.
        explicit SerializeContext(bool registerIntegralTypes = true, bool createEditContext = false);
        virtual ~SerializeContext();

        /// Deleting copy ctor because we own unique_ptr's of IDataContainers
        SerializeContext(const SerializeContext&) = delete;
        SerializeContext& operator =(const SerializeContext&) = delete;

        bool IsTypeReflected(AZ::Uuid typeId) const override;

        /// Create an edit context for this serialize context. If one already exist it will be returned.
        EditContext*    CreateEditContext();
        /// Destroys the internal edit context, all edit related data will be freed. You don't have call DestroyEditContext, it's called when you destroy the serialize context.
        void            DestroyEditContext();
        /// Returns the pointer to the current edit context or NULL if one was not created.
        EditContext*    GetEditContext() const;

        /**
        * \anchor SerializeBind
        * \name Code to bind classes and variables for serialization.
        * For details about what can you expose in a class and what options you have \ref SerializeContext::ClassBuilder
        * @{
        */
        template<class T, class... TBaseClasses>
        ClassBuilder    Class();

        /**
         * When a default object factory can't be used, example a singleton class with private constructors or non default constructors
         * you will have to provide a custom factory.
         */
        template<class T, class... TBaseClasses>
        ClassBuilder    Class(IObjectFactory* factory);

        //! Exposes a C++ enum type to the SerializeContext
        //! The @ref SerializeContext::ClassBuilder has the list of supported options for exposing Enum Type data
        //! The primary function on the EnumBuilder class for adding an Enum value is @ref EnumBuilder::Value()
        template<typename EnumType>
        EnumBuilder Enum();
        template<typename EnumType>
        EnumBuilder Enum(IObjectFactory* factory);

        //! Function Pointer which is used to construct an AZStd::any for a registered type using the Serialize Context
        using CreateAnyFunc = AZStd::any(*)(SerializeContext*);
        //! Function object which is used to create an instance of an ActionHandler function which can be supplied
        //! to the AZStd::any::type_info handler object to allow type erased operations such as construct/destruct, copy/move
        using CreateAnyActionHandler = AZStd::function<AZStd::any::action_handler_for_t(SerializeContext* serializeContext)>;

        //! Allows registration of a TypeId without the need to supply a C++ type
        //! If the type is not already registered, then the ClassData is moved into the SerializeContext
        //! internal structure
        ClassBuilder RegisterType(const AZ::TypeId& typeId, AZ::SerializeContext::ClassData&& classData,
            CreateAnyFunc createAnyFunc = [](SerializeContext*) -> AZStd::any { return {}; });

        //! Unregister a type from the SerializeContext and removes all internal mappings
        //! @return true if the type was previously registered
        bool UnregisterType(const AZ::TypeId& typeId);
        // Helper method that gets the generic info of ValueType and calls Reflect on it, should it exist
        template <class ValueType>
        void RegisterGenericType();

        // Deprecate a previously reflected class so that on load any instances will be silently dropped without the need
        // to keep the original class around.
        // Use of this function should be considered temporary and should be removed as soon as possible.
        void            ClassDeprecate(const char* name, const AZ::Uuid& typeUuid, VersionConverter converter = nullptr);
        // @}

        /**
         * Debug stack element when we enumerate a class hierarchy so we can report better errors!
         */
        struct DbgStackEntry
        {
            void  ToString(AZStd::string& str) const;
            const void*         m_dataPtr;
            Uuid                m_uuid;
            const ClassData*    m_classData;
            const char*         m_elementName;
            const ClassElement* m_classElement;
        };

        class ErrorHandler
        {
        public:

            AZ_CLASS_ALLOCATOR(ErrorHandler, AZ::SystemAllocator);

            ErrorHandler()
                : m_nErrors(0)
                , m_nWarnings(0)
            {
            }

            /// Report an error within the system
            void            ReportError(const char* message);
            /// Report a non-fatal warning within the system
            void            ReportWarning(const char* message);
            /// Pushes an entry onto debug stack.
            void            Push(const DbgStackEntry& de);
            /// Pops the last entry from debug stack.
            void            Pop();
            /// Get the number of errors reported.
            unsigned int    GetErrorCount() const           { return m_nErrors; }
            /// Get the number of warnings reported.
            unsigned int    GetWarningCount() const         { return m_nWarnings; }
            /// Reset error counter
            void            Reset();

        private:

            AZStd::string GetStackDescription() const;

            using DbgStack = AZStd::vector<DbgStackEntry>;
            DbgStack        m_stack;
            unsigned int    m_nErrors;
            unsigned int    m_nWarnings;
        };

        /**
        * \anchor Enumeration
        * \name Function to enumerate the hierarchy of an instance using the serialization info.
        * @{
        */
        enum EnumerationAccessFlags
        {
            ENUM_ACCESS_FOR_READ    = 0,        // you only need read access to the enumerated data
            ENUM_ACCESS_FOR_WRITE   = 1 << 0,   // you need write access to the enumerated data
            ENUM_ACCESS_HOLD        = 1 << 1    // you want to keep accessing the data after enumeration completes. you are responsible for notifying the data owner when you no longer need access.
        };

        /// EnumerateInstance() calls this callback for each node in the instance hierarchy. Return true to also enumerate the node's children, otherwise false.
        using BeginElemEnumCB = AZStd::function< bool (void* /* instance pointer */, const ClassData* /* classData */, const ClassElement* /* classElement*/) >;
        /// EnumerateInstance() calls this callback when enumeration of an element's sub-branch has completed. Return true to continue enumeration of the element's siblings, otherwise false.
        using EndElemEnumCB = AZStd::function< bool () >;

        /**
         * Call this function to traverse an instance's hierarchy by providing address and classId, if you have the typed pointer you can just call \ref EnumerateObject
         * \param ptr pointer to the object for traversal
         * \param classId classId of object for traversal
         * \param beginElemCB callback when we begin/open a child element
         * \param endElemCB callback when we end/close a child element
         * \param accessFlags \ref EnumerationAccessFlags
         * \param classData pointer to the class data for the traversed object to avoid calling FindClassData(classId) (can be null)
         * \param classElement pointer to class element (null for root elements)
         * \param errorHandler optional pointer to the error handler.
         */
        bool EnumerateInstanceConst(EnumerateInstanceCallContext* callContext, const void* ptr, const Uuid& classId, const ClassData* classData, const ClassElement* classElement) const;
        bool EnumerateInstance(EnumerateInstanceCallContext* callContext, void* ptr, Uuid classId, const ClassData* classData, const ClassElement* classElement) const;

        // Deprecated overloads for EnumerateInstance*. Prefer versions that take a \ref EnumerateInstanceCallContext directly.
        bool EnumerateInstanceConst(const void* ptr, const Uuid& classId, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, const ClassData* classData, const ClassElement* classElement, ErrorHandler* errorHandler = nullptr) const;
        bool EnumerateInstance(void* ptr, const Uuid& classId, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, const ClassData* classData, const ClassElement* classElement, ErrorHandler* errorHandler = nullptr) const;

        /// Traverses a give object \ref EnumerateInstance assuming that object is a root element (not classData/classElement information).
        template<class T>
        bool EnumerateObject(T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler = nullptr) const;

        template<class T>
        bool EnumerateObject(const T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler = nullptr) const;

        /// TypeInfo enumeration callback. Return true to continue enumeration, otherwise false.
        typedef AZStd::function< bool(const ClassData* /*class data*/, const Uuid& /* typeId used only when RTTI is triggered, 0 the rest of the time */) > TypeInfoCB;
        /// Enumerate all derived classes of classId type in addition we use the typeId to check any rtti possible match.
        void EnumerateDerived(const TypeInfoCB& callback, const Uuid& classId = Uuid::CreateNull(), const Uuid& typeId = Uuid::CreateNull()) const;
        /// Enumerate all base classes of classId type.
        void EnumerateBase(const TypeInfoCB& callback, const Uuid& classId);
        template<class T>
        void EnumerateDerived(const TypeInfoCB& callback);
        template<class T>
        void EnumerateBase(const TypeInfoCB& callback);

        void EnumerateAll(const TypeInfoCB& callback, bool includeGenerics=false) const;
        // @}

        /// Makes a copy of obj. The behavior is the same as if obj was first serialized out and the copy was then created from the serialized data.
        template<class T>
        T* CloneObject(const T* obj);
        void* CloneObject(const void* ptr, const Uuid& classId);

        template<class T>
        void CloneObjectInplace(T& dest, const T* obj);
        void CloneObjectInplace(void* dest, const void* ptr, const Uuid& classId);

        // Adds forward declarations of inner classes used for fixing data pataches
        // in .uicanvas files
        enum DataPatchUpgradeType : u8;

        class DataPatchUpgrade;
        struct NodeUpgradeSortFunctor;
        class DataPatchUpgradeHandler;
        class DataPatchNameUpgrade;

        // Forward declare DataPatchTypeUpgrade class
        template<class FromT, class ToT>
        class DataPatchTypeUpgrade;

        // A list of node upgrades, sorted in order of the version they convert to
        using DataPatchUpgradeList = AZStd::set<DataPatchUpgrade*, NodeUpgradeSortFunctor>;
        // A sorted mapping of upgrade lists, keyed by (and sorted on) the version they convert from
        using DataPatchUpgradeMap = AZStd::map<unsigned int, DataPatchUpgradeList>;
        // Stores sets of node upgrades keyed by the field they apply to
        using DataPatchFieldUpgrades = AZStd::unordered_map<AZ::Crc32, DataPatchUpgradeMap>;


        /**
         * Helper for directly comparing two instances of a given type.
         * Intended for use in implementations of IDataSerializer::CompareValueData.
         */
        template<typename T>
        struct EqualityCompareHelper
        {
            static bool CompareValues(const void* lhs, const void* rhs)
            {
                const T* dataLhs = reinterpret_cast<const T*>(lhs);
                const T* dataRhs = reinterpret_cast<const T*>(rhs);
                return (*dataLhs == *dataRhs);
            }
        };

        /// Find a class data (stored information) based on a class ID and possible parent class data.
        const ClassData* FindClassData(const Uuid& classId, const SerializeContext::ClassData* parent = nullptr, u32 elementNameCrc = 0) const;

        /// Find a class data (stored information) based on a class name
        AZStd::vector<AZ::Uuid> FindClassId(const AZ::Crc32& classNameCrc) const;

        /// Find GenericClassData data based on the supplied class ID
        GenericClassInfo* FindGenericClassInfo(const Uuid& classId) const;

        /// Creates an AZStd::any based on the provided class Uuid, or returns an empty AZStd::any if no class data is found or the class is virtual
        AZStd::any CreateAny(const Uuid& classId);

        /// Register GenericClassInfo with the SerializeContext
        void RegisterGenericClassInfo(const AZ::Uuid& typeId, GenericClassInfo* genericClassInfo, const CreateAnyFunc& createAnyFunc);

        /// Unregisters all GenericClassInfo instances registered in the current module and deletes the GenericClassInfo instances
        void CleanupModuleGenericClassInfo();

        /**
         * Checks if a type can be downcast to another using Uuid and AZ_RTTI.
         * All classes must be registered with the system.
         */
        bool  CanDowncast(const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper = nullptr, const IRttiHelper* toClassHelper = nullptr) const;

        /**
         * Offsets a pointer from a derived class to a common base class
         * All classes must be registered with the system otherwise a NULL
         * will be returned.
         */
        void* DownCast(void* instance, const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper = nullptr, const IRttiHelper* toClassHelper = nullptr) const;

        /**
         * Casts a pointer using the instance pointer and its class id.
         * In order for the cast to succeed, both types must either support azrtti or be reflected
         * with the serialization context.
         */
        template<class T>
        T Cast(void* instance, const Uuid& instanceClassId) const;

        void RegisterDataContainer(AZStd::unique_ptr<IDataContainer> dataContainer);

        /* Retrieve the underlying typeid if the supplied TypeId represents a C++ enum type
         * and the enum type is registered with the SerializeContext
         * otherwise the supplied typeId is returned
        */
        AZ::TypeId GetUnderlyingTypeId(const TypeId& enumTypeId) const;

    private:

        /// Enumerate function called to enumerate an azrtti hierarchy
        static void EnumerateBaseRTTIEnumCallback(const Uuid& id, void* userData);

        /// Remove class data
        void RemoveClassData(ClassData* classData);
        /// Removes the GenericClassInfo from the GenericClassInfoMap
        void RemoveGenericClassInfo(GenericClassInfo* genericClassInfo);

        /// Adds class data, including base class element data
        template<class T, class BaseClass>
        void AddClassData(ClassData* classData, size_t index);
        template<class T, class...TBaseClasses>
        void AddClassData(ClassData* classData);

        /// Object cloning callbacks.
        bool BeginCloneElement(void* ptr, const ClassData* classData, const ClassElement* elementData, void* stackData, ErrorHandler* errorHandler, AZStd::vector<char>* scratchBuffer);
        bool BeginCloneElementInplace(void* rootDestPtr, void* ptr, const ClassData* classData, const ClassElement* elementData, void* stackData, ErrorHandler* errorHandler, AZStd::vector<char>* scratchBuffer);
        bool EndCloneElement(void* stackData);

        /**
         * Internal structure to maintain class information while we are describing a class.
         * User should call variety of functions to describe class features and data.
         * example:
         *  struct MyStruct {
         *      int m_data };
         *
         * expose for serialization
         *  serializeContext->Class<MyStruct>()
         *      ->Version(3,&MyVersionConverter)
         *      ->Field("data",&MyStruct::m_data);
         */

        // Non-templated functions which can reflect or unreflect a class to the SerializeContext
        using DeprecatedNameVisitWrapper = void(*)(const DeprecatedTypeNameCallback&);
        ClassBuilder ReflectClassInternal(const char* className, const AZ::TypeId& classId,
            IObjectFactory* factory, const DeprecatedNameVisitWrapper& callback,
            IRttiHelper* rttiHelper, CreateAnyFunc createAnyFunc,
            CreateAnyActionHandler createAnyActionHandlerFunc);

        ClassBuilder UnreflectClassInternal(const char* className, const AZ::TypeId& classId,
            const DeprecatedNameVisitWrapper& callback
            );
    public:
        class ClassBuilder
        {
            friend class SerializeContext;
            ClassBuilder(SerializeContext* context, const UuidToClassMap::iterator& classMapIter);
            SerializeContext*           m_context;
            UuidToClassMap::iterator    m_classData;
            AZStd::vector<AttributeSharedPair, AZStdFunctorAllocator>* m_currentAttributes = nullptr;
        public:
            ~ClassBuilder();
            ClassBuilder* operator->()  { return this; }

            /// Declare class field with address of the variable in the class
            template<class ClassType, class FieldType>
            ClassBuilder* Field(const char* name, FieldType ClassType::* address, AZStd::initializer_list<AttributePair> attributeIds = {});

            /* Declare a type change of a serialized field
             *  These are used by the serializer to repair old data patches
             *  Type upgrades will be applied before Name Upgrades. If a type and name change happen concurrently, the field name
             *  given to the type converter should be the old name.
             */
            template <class FromT, class ToT>
            ClassBuilder* TypeChange(AZStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion, AZStd::function<ToT(const FromT&)> upgradeFunc);

            /* Declare a name change of a serialized field
             *  These are used by the serializer to repair old data patches
             *
             */
            ClassBuilder* NameChange(unsigned int fromVersion, unsigned int toVersion, AZStd::string_view oldFieldName, AZStd::string_view newFieldName);

            /**
             * Declare a class field that belongs to a base class of the class. If you use this function to reflect you base class
             * you can't reflect the entire base class.
             */
            template<class ClassType, class BaseType, class FieldType>
            ClassBuilder* FieldFromBase(const char* name, FieldType BaseType::* address);

            /// Add version control to the structure with optional converter. If not controlled a version of 0 is assigned.
            ClassBuilder* Version(unsigned int version, VersionConverter converter = nullptr);

            /// For data types (usually base types) or types that we handle the full serialize, implement this interface.
            /// Takes ownership of the supplied serializer
            ClassBuilder* Serializer(Serialize::IDataSerializerPtr serializer);

            /// For data types (usually base types) or types that we handle the full serialize, implement this interface.
            ClassBuilder* Serializer(IDataSerializer* serializer);

            /// Helper function to create a static instance of specific serializer implementation. \ref Serializer(IDataSerializer*)
            template<typename SerializerImplementation>
            ClassBuilder* Serializer()
            {
                return Serializer(&Serialize::StaticInstance<SerializerImplementation>::s_instance);
            }

            /// For class type that are empty, we want the serializer to create on load, but have no child elements.
            ClassBuilder* SerializeWithNoData();

            /**
             * Implement and provide interface for event handling when necessary.
             * An example for this will be when the class does some thread work and need to know when
             * we are about to access data for serialization (load/save).
             */
            ClassBuilder* EventHandler(IEventHandler* eventHandler);

            /// Helper function to create a static instance of specific event handler implementation. \ref Serializer(IDataSerializer*)
            template<typename EventHandlerImplementation>
            ClassBuilder* EventHandler()
            {
                return EventHandler(&Serialize::StaticInstance<EventHandlerImplementation>::s_instance);
            }

            /// Adds a DataContainer structure for manipulating contained data in custom ways
            ClassBuilder* DataContainer(IDataContainer* dataContainer);

            /// Helper function to create a static instance of the specific data container implementation. \ref DataContainer(IDataContainer*)
            template<typename DataContainerType>
            ClassBuilder* DataContainer()
            {
                return DataContainer(&Serialize::StaticInstance<DataContainerType>::s_instance);
            }

            /**
             * Sets the class persistent ID function getter. This is used when we store the class in containers, so we can identify elements
             * for the purpose of overriding values (aka slices overrides), otherwise the ability to tweak complex types in containers will be limiting
             * due to the lack of reliable ways to address elements in a container.
             */
            ClassBuilder* PersistentId(ClassPersistentId persistentId);

            /**
             * Provide a function to perform basic yes/no on saving. We don't recommend using such pattern as
             * it's very easy to create asymmetrical serialization and lose silently data.
             */
            ClassBuilder* SerializerDoSave(ClassDoSave isSave);

            /**
             * All T (attribute value) MUST be copy or move constructible as they are stored in internal
             * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
             * Attributes can be assigned to either classes or DataElements.
             */
            template <class T>
            ClassBuilder* Attribute(const char* id, T&& value)
            {
                return Attribute(Crc32(id), AZStd::forward<T>(value));
            }

            /**
             * All T (attribute value) MUST be copy or move constructible as they are stored in internal
             * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
             * Attributes can be assigned to either classes or DataElements.
             */
            template <class T>
            ClassBuilder* Attribute(Crc32 idCrc, T&& value);

        private:
            ClassBuilder* FieldImpl(AZStd::initializer_list<AttributePair> attributeIds,
                const char* className, const AZ::TypeId& classId,
                const char* fieldName, const AZ::TypeId& fieldTypeId,
                size_t fieldOffset, size_t fieldSize, bool fieldIsPointer, bool fieldIsEnum,
                const AZ::TypeId& fieldUnderlyingTypeId, IRttiHelper* fieldRttiHelper,
                GenericClassInfo* fieldGenericClassInfo, CreateAnyFunc fieldCreateAnyFunc);
        };

        /**
         * Internal structure to maintain enum information while we are describing a enum type.
         * This can be used to name the values of the enumeration for serialization.
         * example:
         * enum MyEnum
         * {
         *     First,
         *     Second,
         *     Fourth = 4
         * };
         *
         * expose for serialization
         *  serializeContext->Enum<MyEnum>()
         *      ->Value("First",&MyEnum::First)
         *      ->Value("Second",&MyEnum::Second)
         *      ->Value("Fourth",&MyEnum::Fourth);
         */
        class EnumBuilder
        {
            friend class SerializeContext;
            EnumBuilder(SerializeContext* context, const UuidToClassMap::iterator& classMapIter);
        public:
            ~EnumBuilder() = default;
            auto operator->() -> EnumBuilder*;

            //! Declare enum field with a specific value
            template<class EnumType>
            auto Value(const char* name, EnumType value)->EnumBuilder*;

            //! Add version control to the structure with optional converter. If not controlled a version of 0 is assigned.
            auto Version(unsigned int version, VersionConverter converter = nullptr) -> EnumBuilder*;

            //! For data types (usually base types) or types that we handle the full serialize, implement this interface.
            auto Serializer(Serialize::IDataSerializerPtr serializer) -> EnumBuilder*;

            //! Helper function to create a static instance of specific serializer implementation. \ref Serializer(IDataSerializer*)
            template<typename SerializerImplementation>
            auto Serializer()->EnumBuilder*;

            /**
             * Implement and provide interface for event handling when necessary.
             * An example for this will be when the class does some thread work and need to know when
             * we are about to access data for serialization (load/save).
             */
            auto EventHandler(IEventHandler* eventHandler) -> EnumBuilder*;

            //! Helper function to create a static instance of specific event handler implementation. \ref Serializer(IDataSerializer*)
            template<typename EventHandlerImplementation>
            auto EventHandler()->EnumBuilder*;

            //! Adds a DataContainer structure for manipulating contained data in custom ways
            auto DataContainer(IDataContainer* dataContainer) -> EnumBuilder*;

            //! Helper function to create a static instance of the specific data container implementation. \ref DataContainer(IDataContainer*)
            template<typename DataContainerType>
            auto DataContainer()->EnumBuilder*;

            /**
             * Sets the class persistent ID function getter. This is used when we store the class in containers, so we can identify elements
             * for the purpose of overriding values (aka slices overrides), otherwise the ability to tweak complex types in containers will be limiting
             * due to the lack of reliable ways to address elements in a container.
             */
            auto PersistentId(ClassPersistentId persistentId) -> EnumBuilder*;

            /**
             * All T (attribute value) MUST be copy or move constructible as they are stored in internal
             * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
             * Attributes can be assigned to either classes or DataElements.
             */
            template <class T>
            auto Attribute(Crc32 idCrc, T&& value) -> EnumBuilder*;

        private:
            SerializeContext* m_context;
            UuidToClassMap::iterator m_classData;
            AZStd::vector<AttributeSharedPair, AZStdFunctorAllocator>* m_currentAttributes = nullptr;
        };

    private:
        EditContext* m_editContext;  ///< Pointer to optional edit context.
        UuidToClassMap  m_uuidMap;      ///< Map for all class in this serialize context
        AZStd::unordered_multimap<AZ::Crc32, AZ::Uuid> m_classNameToUuid;  ///< Map all class names to their uuid
        AZStd::unordered_multimap<AZ::Crc32, AZ::Uuid> m_deprecatedNameToTypeIdMap;  ///< Stores a mapping of deprecated type names that a type exposes through the AzDeprecatedTypeNameVisitor
        AZStd::unordered_multimap<Uuid, GenericClassInfo*>  m_uuidGenericMap;      ///< Uuid to ClassData map of reflected classes with GenericTypeInfo
        AZStd::unordered_multimap<Uuid, Uuid> m_legacySpecializeTypeIdToTypeIdMap; ///< Keep a map of old legacy specialized typeids of template classes to new specialized typeids
        AZStd::unordered_map<Uuid, CreateAnyFunc>  m_uuidAnyCreationMap;      ///< Uuid to Any creation function map
        AZStd::unordered_map<TypeId, TypeId> m_enumTypeIdToUnderlyingTypeIdMap; ///< Uuid to keep track of the correspond underlying type id for an enum type that is reflected as a Field within the SerializeContext
        AZStd::vector<AZStd::unique_ptr<IDataContainer>> m_dataContainers; ///< Takes care of all related IDataContainer's lifetimes

        class PerModuleGenericClassInfo;
        AZStd::unordered_set<PerModuleGenericClassInfo*>  m_perModuleSet; ///< Stores the static PerModuleGenericClass structures keeps track of reflected GenericClassInfo per module

        friend PerModuleGenericClassInfo& GetCurrentSerializeContextModule();
    };

    SerializeContext::PerModuleGenericClassInfo& GetCurrentSerializeContextModule();
} // namespace AZ

namespace AZ
{
    // Types listed earlier here will have higher priority
    enum SerializeContext::DataPatchUpgradeType : u8
    {
        TYPE_UPGRADE,
        NAME_UPGRADE
    };

    // Base type used for single-node version upgrades
    class SerializeContext::DataPatchUpgrade
    {
    public:
        AZ_CLASS_ALLOCATOR(DataPatchUpgrade, SystemAllocator);
        AZ_TYPE_INFO_WITH_NAME_DECL(DataPatchUpgrade);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        DataPatchUpgrade(AZStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion);
        virtual ~DataPatchUpgrade() = default;

        // This will only check to see if this has the same upgrade type and is applied to the same field.
        // Deeper comparisons are left to the individual upgrade types.
        bool operator==(const DataPatchUpgrade& RHS) const;

        // Used to sort nodes upgrades.
        bool operator<(const DataPatchUpgrade& RHS) const;

        virtual void Apply(AZ::SerializeContext& /*context*/, SerializeContext::DataElementNode& /*elementNode*/) const {}

        unsigned int FromVersion() const;
        unsigned int ToVersion() const;

        const AZStd::string& GetFieldName() const;
        AZ::Crc32 GetFieldCRC() const;

        DataPatchUpgradeType GetUpgradeType() const;

        // Type Upgrade Interface Functions
        virtual AZ::TypeId GetFromType() const { return AZ::TypeId::CreateNull(); }
        virtual AZ::TypeId GetToType() const { return AZ::TypeId::CreateNull(); }

        virtual AZStd::any Apply(const AZStd::any& in) const { return in; }

        // Name upgrade interface functions
        virtual AZStd::string GetNewName() const { return {}; }

    protected:
        AZStd::string m_targetFieldName;
        AZ::Crc32 m_targetFieldCRC;
        unsigned int m_fromVersion;
        unsigned int m_toVersion;

        DataPatchUpgradeType m_upgradeType;
    };

    // Binary predicate for ordering per-version upgrades
    // When multiple upgrades exist from a particular version, we only want
    // to apply the one that upgrades to the maximum possible version.
    struct SerializeContext::NodeUpgradeSortFunctor
    {
        // Provides sorting of lists of node upgrade pointers
        bool operator()(const DataPatchUpgrade* LHS, const DataPatchUpgrade* RHS)
        {
            if (LHS->ToVersion() == RHS->ToVersion())
            {
                AZ_Assert(LHS->GetUpgradeType() != RHS->GetUpgradeType(), "Data Patch Upgrades with the same from/to version numbers must be different types.");

                if (LHS->GetUpgradeType() == NAME_UPGRADE)
                {
                    return true;
                }

                return false;
            }
            return LHS->ToVersion() < RHS->ToVersion();
        }
    };

    // A class to maintain and apply all of the per-field node upgrades that apply to one single field.
    // Performs error checking when building the field array, manages the lifetime of the upgrades, and
    // deals with application of the upgrades to both nodes and raw values.
    class SerializeContext::DataPatchUpgradeHandler
    {
    public:
        DataPatchUpgradeHandler()
        {}

        ~DataPatchUpgradeHandler();

        // This function assumes ownership of the upgrade
        void AddFieldUpgrade(DataPatchUpgrade* upgrade);

        const DataPatchFieldUpgrades& GetUpgrades() const;

    private:
        DataPatchFieldUpgrades m_upgrades;
    };

    class SerializeContext::DataPatchNameUpgrade : public DataPatchUpgrade
    {
    public:
        AZ_CLASS_ALLOCATOR(DataPatchNameUpgrade, SystemAllocator);
        AZ_TYPE_INFO_WITH_NAME_DECL(DataPatchNameUpgrade);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        DataPatchNameUpgrade(unsigned int fromVersion, unsigned int toVersion, AZStd::string_view oldName, AZStd::string_view newName)
            : DataPatchUpgrade(oldName, fromVersion, toVersion)
            , m_newNodeName(newName)
        {
            m_upgradeType = NAME_UPGRADE;
        }

        ~DataPatchNameUpgrade() override = default;

        // The equivalence operator is used to determine if upgrades are functionally equivalent.
        // Two upgrades are considered equivalent (Incompatible) if they operate on the same field,
        // are the same type of upgrade, and upgrade from the same version.
        bool operator<(const DataPatchUpgrade& RHS) const;
        bool operator<(const DataPatchNameUpgrade& RHS) const;

        void Apply(AZ::SerializeContext& context, SerializeContext::DataElementNode& node) const override;
        using DataPatchUpgrade::Apply;

        AZStd::string GetNewName() const override;

    private:
        AZStd::string m_newNodeName;
    };
    // As the SerializeContext::DataPatchTypeUpgrade class was forward declared
    // in the SerializeContext class definition, TypeInfo GetO3de* functions can be now declared as well
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_DECL(SerializeContext::DataPatchTypeUpgrade, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS);

    template<class FromT, class ToT>
    class SerializeContext::DataPatchTypeUpgrade : public DataPatchUpgrade
    {
    public:
        AZ_CLASS_ALLOCATOR(DataPatchTypeUpgrade, SystemAllocator);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        DataPatchTypeUpgrade(AZStd::string_view nodeName, unsigned int fromVersion, unsigned int toVersion, AZStd::function<ToT(const FromT& data)> upgradeFunc)
            : DataPatchUpgrade(nodeName, fromVersion, toVersion)
            , m_upgrade(AZStd::move(upgradeFunc))
            , m_fromTypeID(AZ::AzTypeInfo<FromT>::Uuid())
            , m_toTypeID(AZ::AzTypeInfo<ToT>::Uuid())
        {
            m_upgradeType = TYPE_UPGRADE;
        }

        ~DataPatchTypeUpgrade() override = default;

        bool operator<(const DataPatchTypeUpgrade& RHS) const
        {
            return DataPatchUpgrade::operator<(RHS);
        }

        AZStd::any Apply(const AZStd::any& in) const override
        {
            const FromT& inValue = AZStd::any_cast<const FromT&>(in);
            return AZStd::any(m_upgrade(inValue));
        }

        void Apply(AZ::SerializeContext& context, SerializeContext::DataElementNode& node) const override;

        AZ::TypeId GetFromType() const override
        {
            return m_fromTypeID;
        }

        AZ::TypeId GetToType() const override
        {
            return m_toTypeID;
        }

    private:
        AZStd::function<ToT(const FromT& data)> m_upgrade;

        // Used for comparison of upgrade functions to determine uniqueness
        AZ::TypeId m_fromTypeID;
        AZ::TypeId m_toTypeID;
    };
} // namespace AZ

namespace AZ::Serialize
{
    /**
     * Class element. When a class doesn't have a direct serializer,
     * he is an aggregation of ClassElements (which can be another classes).
     */
    struct ClassElement
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(ClassElement);
        enum Flags
        {
            FLG_POINTER = (1 << 0),       ///< Element is stored as pointer (it's not a value).
            FLG_BASE_CLASS = (1 << 1),       ///< Set if the element is a base class of the holding class.
            FLG_NO_DEFAULT_VALUE = (1 << 2),       ///< Set if the class element can't have a default value.
            FLG_DYNAMIC_FIELD = (1 << 3),       ///< Set if the class element represents a dynamic field (DynamicSerializableField::m_data).
            FLG_UI_ELEMENT = (1 << 4),       ///< Set if the class element represents a UI element tied to the ClassData of its parent.
        };

        enum class AttributeOwnership
        {
            Parent, // Attributes should be deleted when the ClassData containing us is destroyed
            Self,   // Attributes should be deleted when we are destroyed
            None,   // Attributes should never be deleted by us, their lifetime is managed somewhere else
        };

        ~ClassElement();

        ClassElement& operator=(const ClassElement& other);

        void ClearAttributes();
        Attribute* FindAttribute(AttributeId attributeId) const;

        const char* m_name{ "" };                 ///< Used in XML output and debugging purposes
        u32 m_nameCrc{};                          ///< CRC32 of m_name
        Uuid m_typeId = AZ::TypeId::CreateNull();
        size_t m_dataSize{};
        size_t m_offset{};

        IRttiHelper* m_azRtti{};                  ///< Interface used to support RTTI.
        GenericClassInfo* m_genericClassInfo = nullptr;   ///< Valid when the generic class is set. So you don't search for the actual type in the class register.
        Edit::ElementData* m_editData{};          ///< Pointer to edit data (generated by EditContext).
        AZStd::vector<AttributeSharedPair, AZStdFunctorAllocator> m_attributes{
            AZStdFunctorAllocator([]() -> IAllocator& { return AZ::AllocatorInstance<AZ::SystemAllocator>::Get(); })
        }; ///< Attributes attached to ClassElement. Lambda is required here as AZStdFunctorAllocator expects a function pointer
        ///< that returns an IAllocator& and the AZ::AllocatorInstance<AZ::SystemAllocator>::Get returns an AZ::SystemAllocator&
        /// which while it inherits from IAllocator, does not work as function pointers do not support covariant return types
        AttributeOwnership m_attributeOwnership = AttributeOwnership::Parent;
        int m_flags{};    ///<
    };

    /**
     * Class Data contains the data/info for each registered class
     * all if it members (their offsets, etc.), creator, version converts, etc.
     */
    class ClassData
    {
        friend SerializeContext;
        using ClassElementArray = AZStd::vector<ClassElement>;
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(ClassData);

        ClassData();
        ~ClassData() { ClearAttributes(); }
        ClassData(ClassData&&) = default;
        ClassData& operator=(ClassData&&) = default;
        template<class T>
        static ClassData Create(const char* name, const Uuid& typeUuid, IObjectFactory* factory, IDataSerializer* serializer = nullptr, IDataContainer* container = nullptr);

        bool    IsDeprecated() const { return m_version == Serialize::VersionClassDeprecated; }
        void    ClearAttributes();
        Attribute* FindAttribute(AttributeId attributeId) const;
        ///< Checks if the supplied typeid can be converted to an instance of the class represented by this Class Data
        ///< @param convertibleTypeId type to check to determine if it can converted to an element of class represent by this Class Data
        ///< @return if the classData can store the convertible type element true is returned
        bool CanConvertFromType(const TypeId& convertibleTypeId, AZ::SerializeContext& serializeContext) const;
        ///< Retrieves a memory address that can be used store an element of the convertible type
        ///< @param resultPtr output parameter that is populated with the memory address that can be used to store an element of the convertible type
        ///< @param convertibleTypeId type to check to determine if it can converted to an element of class represent by this Class Data
        ///< @param classPtr memory address of the class represented by the ClassData
        ///< @return true if a non-null memory address has been returned that can store the  convertible type
        bool ConvertFromType(void*& convertibleTypePtr, const TypeId& convertibleTypeId, void* classPtr, AZ::SerializeContext& serializeContext) const;

        /// Find the persistence id (check base classes) \todo this is a TEMP fix, analyze and cache that information in the class
        SerializeContext::ClassPersistentId GetPersistentId(const SerializeContext& context) const;

        const char* m_name;
        Uuid                m_typeId;
        unsigned int        m_version;          ///< Data version (by default 0)
        SerializeContext::VersionConverter    m_converter;        ///< Data version converter, a static member that should not need an instance to convert it's data.
        IObjectFactory* m_factory;          ///< Interface for object creation.
        SerializeContext::ClassPersistentId   m_persistentId;     ///< Function to retrieve class instance persistent Id.
        SerializeContext::ClassDoSave         m_doSave;           ///< Function what will choose to Save or not an instance.
        IDataSerializerPtr  m_serializer;       ///< Interface for actual data serialization. If this is not NULL m_elements must be empty.
        IEventHandler* m_eventHandler;     ///< Optional interface for Event notification (start/stop serialization, etc.)

        IDataContainer* m_container;        ///< Interface if this class represents a data container. Data will be accessed using this interface.
        IRttiHelper* m_azRtti;           ///< Interface used to support RTTI. Set internally based on type provided to Class<T>.
        IDataConverter* m_dataConverter{};    ///< Interface used to convert unrelated types to elements of this class

        Edit::ClassData* m_editData;         ///< Edit data for the class display.
        ClassElementArray   m_elements;         ///< Sub elements. If this is not empty m_serializer should be NULL (there is no point to have sub-elements, if we can serialize the entire class).

        // A collection of single-node upgrades to apply during serialization
        // The map is keyed by the version the upgrades are converting from
        // Upgrades are then sorted in the order of the version they upgrade to
        SerializeContext::DataPatchUpgradeHandler m_dataPatchUpgrader;

        ///< Attributes for this class type. Lambda is required here as AZStdFunctorAllocator expects a function pointer
        ///< that returns an IAllocator& and the AZ::AllocatorInstance<AZ::SystemAllocator>::Get returns an AZ::SystemAllocator&
        /// which while it inherits from IAllocator, does not work as function pointers do not support covariant return types
        AZStd::vector<AttributeSharedPair, AZStdFunctorAllocator> m_attributes{ AZStdFunctorAllocator(&GetSystemAllocator) };

        //! Functor object which can be used to initialize the AZStd::any::type_info in order
        //! to allow it to construct, destruct, copy/move instances of the stored type
        //! in a type-erased manner
        //! This can be used with the `any(const void* pointer, const type_info& typeInfo)` extension constructor
        //! to construct an AZStd::any without access to the C++ type at that point
        //! @param pointer to SerializeContext which is used for type-erased operations for non-copyable types
        SerializeContext::CreateAnyActionHandler m_createAzStdAnyActionHandler;

    private:
        static ClassData CreateImpl(const char* name, const Uuid& typeUuid, IObjectFactory* factory,
            IDataSerializer* serializer, IDataContainer* container,
            IRttiHelper* rttiHelper, SerializeContext::CreateAnyActionHandler createAzStdAnyActionHandler);
        static IAllocator& GetSystemAllocator()
        {
            return AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        }
    };

    /**
     * Interface for creating and destroying object from the serializer.
     */
    class IObjectFactory
    {
    public:

        virtual ~IObjectFactory() {}

        /// Called to create an instance of an object.
        virtual void* Create(const char* name) = 0;

        /// Called to destroy an instance of an object
        virtual void  Destroy(void* ptr) = 0;
        void Destroy(const void* ptr)
        {
            Destroy(const_cast<void*>(ptr));
        }
    };

    /**
     * Interface for data serialization. Should be implemented for lowest level
     * of data. Once this implementation is detected, the class will not be drilled
     * down. We will assume this implementation covers the full class.
     */
    class IDataSerializer
    {
    public:
        static IDataSerializerDeleter CreateDefaultDeleteDeleter();
        static IDataSerializerDeleter CreateNoDeleteDeleter();

        virtual ~IDataSerializer() {}

        /// Store the class data into a stream.
        virtual size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) = 0;

        /// Load the class data from a stream.
        virtual bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian = false) = 0;

        /// Convert binary data to text.
        virtual size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) = 0;

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        virtual size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) = 0;

        /// Compares two instances of the type.
        /// \return true if they match.
        /// Note: Input pointers are assumed to point to valid instances of the class.
        virtual bool    CompareValueData(const void* lhs, const void* rhs) = 0;

        /// Optional post processing of the cloned data to deal with members that are not serialize-reflected.
        virtual void PostClone(void* /*classPtr*/) {}
    };

    /**
    * Interface for a data container. This might be an AZStd container or just a class with
    * elements defined in some template manner (usually with templates :) )
    */
    class IDataContainer
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(IDataContainer);
        AZ_RTTI_NO_TYPE_INFO_DECL()

        using ElementCB = AZStd::function< bool(void* /* instance pointer */, const Uuid& /*elementClassId*/, const ClassData* /* elementGenericClassData */, const ClassElement* /* genericClassElement */) >;
        using ElementTypeCB = AZStd::function< bool(const Uuid& /*elementClassId*/, const ClassElement* /* genericClassElement */) >;
        virtual ~IDataContainer() {}

        /// Mix-in for associative container actions, implement or provide this to offer key/value actions
        class IAssociativeDataContainer
        {
        protected:
            /// Reserve a key and get its address. Used by CreateKey.
            virtual void* AllocateKey() = 0;
            /// Deallocates a key created by ReserveKey. Used by CreateKey.
            virtual void    FreeKey(void* key) = 0;

        public:
            //! Stores the type structure of container the associative type represents
            //! The valid values are an ordered set(Set, Map) or a hash table structure(UnorderedSet, UnorderedMap)
            enum class AssociativeType
            {
                Unknown,
                Set,
                Map,
                UnorderedSet,
                UnorderedMap
            };
            AZ_TYPE_INFO_WITH_NAME_DECL(IAssociativeDataContainer);
            virtual ~IAssociativeDataContainer() {}

            struct KeyPtrDeleter
            {
                KeyPtrDeleter(IAssociativeDataContainer* associativeDataContainer)
                    : m_associativeDataContainer(associativeDataContainer)
                {}

                void operator()(void* key)
                {
                    m_associativeDataContainer->FreeKey(key);
                }

                IAssociativeDataContainer* m_associativeDataContainer{};
            };

            /// Reserve a key that can be used for associative container operations.
            AZStd::shared_ptr<void> CreateKey()
            {
                return AZStd::shared_ptr<void>(AllocateKey(), KeyPtrDeleter(this));
            }
            //! Get an element's address by its key. Not used for serialization.
            virtual void* GetElementByKey(void* instance, const ClassElement* classElement, const void* key) const = 0;
            //! Populates element with key (for associative containers). Not used for serialization.
            virtual void    SetElementKey(void* element, void* key) const = 0;
            //! Get the mapped value's address by its key. If there is no mapped value (like in a set<>) the value returned is the key itself
            virtual void* GetValueByKey(void* instance, const void* key) const = 0;
            //! Queries the type structure(Set vs. Map, Ordered vs. Hash Table) represented by the AssociativeDataContainer
            virtual AssociativeType GetAssociativeType() const = 0;
        };

        /// Return default element generic name (used by most containers).
        static inline const char* GetDefaultElementName() { return "element"; }
        /// Return default element generic name crc (used by most containers).
        static inline u32 GetDefaultElementNameCrc() { return AZ_CRC_CE("element"); }

        // Returns default element generic name unless overridden by an IDataContainer
        virtual const char* GetElementName([[maybe_unused]] int index = 0) { return GetDefaultElementName(); }
        // Returns default element generic name crc unless overridden by an IDataContainer
        virtual u32 GetElementNameCrC([[maybe_unused]] int index = 0) { return GetDefaultElementNameCrc(); }

        /// Returns the generic element (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
        virtual const ClassElement* GetElement(u32 elementNameCrc) const = 0;
        /// Populates the supplied classElement by looking up the name in the DataElement. Returns true if the classElement was populated successfully
        virtual bool GetElement(ClassElement& classElement, const DataElement& dataElement) const = 0;
        /// Enumerate elements in the container based on the stored entries.
        virtual void    EnumElements(void* instance, const ElementCB& cb) = 0;
        /// Enumerate elements in the container based on possible storage types. If the callback is not called it means there are no restrictions on what can be stored in the container.
        virtual void    EnumTypes(const ElementTypeCB& cb) = 0;
        /// Return number of elements in the container.
        virtual size_t  Size(void* instance) const = 0;
        /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
        virtual size_t  Capacity(void* instance) const = 0;
        /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
        virtual bool    IsStableElements() const = 0;
        /// Returns true if the container is fixed size (not capacity), otherwise false.
        virtual bool    IsFixedSize() const = 0;
        /// Returns if the container is fixed capacity, otherwise false
        virtual bool    IsFixedCapacity() const = 0;
        /// Returns true if the container represents a smart pointer.
        virtual bool    IsSmartPointer() const = 0;
        /// Returns true if elements can be retrieved by index.
        virtual bool    CanAccessElementsByIndex() const = 0;
        /// Returns the associative interface for this container (e.g. the container itself if it inherits it) if available, otherwise null.
        virtual bool SwapElements([[maybe_unused]] void* instance, [[maybe_unused]] size_t firstIndex, [[maybe_unused]] size_t secondIndex)
        {
            return false;
        }
        virtual IAssociativeDataContainer* GetAssociativeContainerInterface() { return nullptr; }
        virtual const IAssociativeDataContainer* GetAssociativeContainerInterface() const { return nullptr; }
        //! Returns true if the IDataContainer represents a reflected sequence type(AZStd array, (fixed)vector, list)
        virtual bool IsSequenceContainer() const { return false; }
        /// Reserve an element and get its address (called before the element is loaded).
        virtual void* ReserveElement(void* instance, const ClassElement* classElement) = 0;
        /// Free an element that was reserved using ReserveElement, but was not stored by calling StoreElement.
        virtual void    FreeReservedElement(void* instance, void* element, SerializeContext* deletePointerDataContext)
        {
            RemoveElement(instance, element, deletePointerDataContext);
        }
        /// Get an element's address by its index (called before the element is loaded).
        virtual void* GetElementByIndex(void* instance, const ClassElement* classElement, size_t index) = 0;
        /// Store the element that was reserved before (called post loading)
        virtual void    StoreElement(void* instance, void* element) = 0;
        /// Remove element in the container. Returns true if the element was removed, otherwise false. If deletePointerDataContext is NOT null, this indicated that you want the remove function to delete/destroy any Elements that are pointer!
        virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) = 0;
        /**
         * Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements).
         * Element should be sorted on address in acceding order. Returns number of elements removed.
         * If deletePointerDataContext is NOT null, this indicates that you want the remove function to delete/destroy any Elements that are pointer,
         */
        virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) = 0;
        /// Clear elements in the instance. If deletePointerDataContext is NOT null, this indicated that you want the remove function to delete/destroy any Elements that are pointer!
        virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) = 0;
        /// Called when elements inside the container have been modified.
        virtual void    ElementsUpdated(void* instance);

    protected:
        /// Free element data (when the class elements are pointers).
        void DeletePointerData(SerializeContext* context, const ClassElement* classElement, const void* element);
    };

    /**
     * Serialize class events.
     * These can be used to hook certain events that occur when reading/writing objects using the Serialize Reflection Context
     * Note that these events will NOT be called if you have installed a custom serializer for your custom class, since
     * in that case, your class will be entirely handled by that custom serializer installed.  This is true for both the ObjectStream (XML, binary)
     * and JSON serializers - using a custom serializer skips this part, so if you do need to do anything special, do it in the custom serializer.
     * IMPORTANT: Serialize events can be called from serialization thread(s). So all functions should be thread safe.
     *
     *  Important note:
     * OnReadBegin and OnReadEnd are not called when using the Json serializer, because the Json serializer's API assumes that
     * reading from C++ objects into JSON has no side effects on the objects being serialized, and thus will not call a non-const
     * callback such as these.
     *
     * The ObjectStream serializer has no problem with this and will in fact const-cast the const ptr fed into it, just so that it can
     * invoke OnReadBegin and OnReadEnd.
     * 
     * OnReadBegin and OnReadEnd are called during serialization - that is, when reading FROM a C++ object INTO an ObjectStream stream,
     * and the purpose of which is to fixup data in the c++ object before / after saving it to a data stream.
     * If you want to fix up data after LOADING it into a C++ object, use OnWriteEnd, not OnReadEnd.
     *
     * Additional Caveat:  It is possible to tell the serialize context to walk a tree of reflected C++ objects and invoke a callback
     * for each element on them - this is, in fact, how serializing works (it walks the tree of reflected objects and serializes them).
     * If you are using this feature, you should be aware that OnReadBegin and OnReadEnd will be called for each element in the tree,
     * since it is "reading from" the objects.  However, if you pass the ENUM_ACCESS_FOR_WRITE flag, it will INSTEAD call OnWriteBegin
     * and OnWriteEnd for the c++ objects it is visiting, despite the fact that you are technically enumerating them.
     */
    class IEventHandler
    {
    public:
        virtual ~IEventHandler() {}

        /// the Read**** functions are called when SERIALIZING.  Do not use them to do post-load fixups.
        /// Called right before we start reading from the instance pointed by classPtr.
        virtual void OnReadBegin(void* classPtr) { (void)classPtr; }
        /// Called after we are done reading from the instance pointed by classPtr.
        virtual void OnReadEnd(void* classPtr) { (void)classPtr; }

        /// The Write**** functions are called when DESERIALIZING.  You can use these to do post-load fixups.
        /// Called right before we start writing to the instance pointed by classPtr.
        virtual void OnWriteBegin(void* classPtr) { (void)classPtr; }
        /// Called after we are done writing to the instance pointed by classPtr.
        /// NOTE: Care must be taken when using this callback. It is called when ID remapping occurs,
        /// an instance is cloned or an instance is loaded from an ObjectStream.
        /// This means that this function can be invoked multiple times in the course of serializing a new instance from an ObjectStream
        /// or cloning an object.
        virtual void OnWriteEnd(void* classPtr) { (void)classPtr; }

        /// PATCHING
        /// These functions will not be called when using the Json serializer, as patching operates entirely differently.
        /// Called right before we start data patching the instance pointed by classPtr.
        virtual void OnPatchBegin(void* classPtr, const DataPatchNodeInfo& patchInfo) { (void)classPtr; (void)patchInfo; }
        /// Called after we are done data patching the instance pointed by classPtr.
        virtual void OnPatchEnd(void* classPtr, const DataPatchNodeInfo& patchInfo) { (void)classPtr; (void)patchInfo; }

        /// Called after an instance has been loaded from a source data stream, such as ObjectStream::Load or JsonSerialization::Load
        virtual void OnLoadedFromObjectStream(void* classPtr) { (void)classPtr; }
        /// Called after an object is cloned in SerializeContext::EndCloneObject
        virtual void OnObjectCloned(void* classPtr) { (void)classPtr; }
    };

    /**
     * Data Converter interface which can be used to provide a conversion operation from to unrelated C++ types
     * derived class to base class casting is taken care of through the RTTI system so those relations should not be
     * check within this class
     */
    class IDataConverter
    {
    public:
        virtual ~IDataConverter() = default;
        ///< Callback to determine if the supplied convertible type can be stored in an instance of classData
        ///< @param convertibleTypeId type to check to determine if it can converted to an element of class represent by this Class Data
        ///< @param classData reference to the metadata representing the type stored in classPtr
        ///< @return if the classData can store the convertible type element true is returned
        virtual bool CanConvertFromType(const TypeId& convertibleTypeId, const SerializeContext::ClassData& classData,
            SerializeContext& /*serializeContext*/);

        ///< Callback that can be used to retrieve a memory address in which to store an element of the supplied convertible type
        ///< @param convertibleTypePtr result pointer that should be populated with an address that can store an element of the convertible type
        ///< @param convertibleTypeId type to check to determine if it can converted to an element of class represent by this Class Data
        ///< @param classPtr memory address of the class represented by the @classData type
        ///< @param classData reference to the metadata representing the type stored in classPtr
        ///< @return true if a non-null memory address has been returned that can store the convertible type
        virtual bool ConvertFromType(void*& convertibleTypePtr, const TypeId& convertibleTypeId, void* classPtr,
            const SerializeContext::ClassData& classData, SerializeContext& /*serializeContext*/);
    };

    /**
     * \anchor VersionControl
     * \name Version Control
     * @{
     */

     /**
      * Represents an element in the tree of serialization data.
      * Each element contains metadata about itself and (possibly) a data value.
      * An element representing an int will have a data value, but an element
      * representing a vector or class will not (their contents are stored in sub-elements).
      */
    struct DataElement
    {
        DataElement();
        ~DataElement();
        DataElement(const DataElement& rhs);
        DataElement& operator = (const DataElement& rhs);
        DataElement(DataElement&& rhs);
        DataElement& operator = (DataElement&& rhs);

        enum DataType
        {
            DT_TEXT,        ///< data points to a string representation of the data
            DT_BINARY,      ///< data points to a binary representation of the data in native endian
            DT_BINARY_BE,   ///< data points to a binary representation of the data in big endian
        };

        const char* m_name;         ///< Name of the parameter, they must be unique with in the scope of the class.
        u32             m_nameCrc;      ///< CRC32 of name
        DataType        m_dataType;     ///< What type of data, if we have any.
        Uuid            m_id = AZ::Uuid::CreateNull();           ///< Reference ID, the meaning can change depending on what are we referencing.
        unsigned int    m_version;  ///< Version of data in the stream. This can be the current version or older. Newer version will be handled internally.
        size_t          m_dataSize; ///< Size of the data pointed by "data" in bytes.

        AZStd::vector<char> m_buffer; ///< Local buffer used by the ByteContainerStream when the DataElement needs to own the data
        IO::ByteContainerStream<AZStd::vector<char> > m_byteStream; ///< Stream used when the DataElement needs to own the data.

        IO::GenericStream* m_stream; ///< Pointer to the stream that holds the element's data, it may point to m_byteStream.
    };

    /**
     * Represents a node in the tree of serialization data.
     * Holds a DataElement describing itself and a list of sub nodes.
     * For example, a class would be represented as a parent node
     * with its member variables in sub nodes.
     */
    class DataElementNode
    {
        friend class AZ::ObjectStreamInternal::ObjectStreamImpl;
        friend class AZ::DataOverlayTarget;

    public:
        DataElementNode()
            : m_classData(nullptr) {}

        /**
         * Get/Set data work only on leaf nodes
         */
        template <typename T>
        bool GetData(T& value, SerializeContext::ErrorHandler* errorHandler = nullptr);
        template <typename T>
        bool GetChildData(u32 childNameCrc, T& value);
        template <typename T>
        bool SetData(SerializeContext& sc, const T& value, SerializeContext::ErrorHandler* errorHandler = nullptr);

        //! @deprecated Use GetData instead
        template <typename T>
        bool GetDataHierarchy(SerializeContext&, T& value, SerializeContext::ErrorHandler* errorHandler = nullptr);

        template <typename T>
        bool FindSubElementAndGetData(AZ::Crc32 crc, T& outValue);

        /**
         * Converts current DataElementNode from one type to another.
         * Keep in mind that if the new "type" has sub-elements (not leaf - serialized element)
         * you will need to add those elements after calling convert.
         */
        bool Convert(SerializeContext& sc, const char* name, const Uuid& id);
        bool Convert(SerializeContext& sc, const Uuid& id);
        template <typename T>
        bool Convert(SerializeContext& sc, const char* name);
        template <typename T>
        bool Convert(SerializeContext& sc);

        DataElement& GetRawDataElement() { return m_element; }
        const DataElement& GetRawDataElement() const { return m_element; }
        u32                 GetName() const { return m_element.m_nameCrc; }
        const char* GetNameString() const { return m_element.m_name; }
        void                SetName(const char* newName);
        unsigned int        GetVersion() const { return m_element.m_version; }
        void                SetVersion(unsigned int version) { m_element.m_version = version; }
        const Uuid& GetId() const { return m_element.m_id; }

        int                 GetNumSubElements() const { return static_cast<int>(m_subElements.size()); }
        DataElementNode& GetSubElement(int index) { return m_subElements[index]; }
        int                 FindElement(u32 crc);
        DataElementNode* FindSubElement(u32 crc);
        void                RemoveElement(int index);
        bool                RemoveElementByName(u32 crc);
        int                 AddElement(const DataElementNode& elem);
        int                 AddElement(SerializeContext& sc, const char* name, const Uuid& id);
        int                 AddElement(SerializeContext& sc, const char* name, const ClassData& classData);
        int                 AddElement(SerializeContext& sc, AZStd::string_view name, GenericClassInfo* genericClassInfo);
        template <typename T>
        int                 AddElement(SerializeContext& sc, const char* name);
        /// \returns index of the replaced element (index) if replaced, otherwise -1
        template <typename T>
        int                 AddElementWithData(SerializeContext& sc, const char* name, const T& dataToSet);
        int                 ReplaceElement(SerializeContext& sc, int index, const char* name, const Uuid& id);
        template <typename T>
        int                 ReplaceElement(SerializeContext& sc, int index, const char* name);

    protected:
        typedef AZStd::vector<DataElementNode> NodeArray;

        bool SetDataHierarchy(SerializeContext& sc, const void* objectPtr, const Uuid& classId, SerializeContext::ErrorHandler* errorHandler = nullptr, const ClassData* classData = nullptr);
        bool GetDataHierarchy(void* objectPtr, const Uuid& classId, SerializeContext::ErrorHandler* errorHandler = nullptr);

        struct DataElementInstanceData
        {
            void* m_ptr = nullptr;
            DataElementNode* m_dataElement = nullptr;
            int m_currentContainerElementIndex = 0;
        };

        using NodeStack = AZStd::list<DataElementInstanceData>;
        bool GetDataHierarchyEnumerate(SerializeContext::ErrorHandler* errorHandler, NodeStack& nodeStack);
        bool GetClassElement(ClassElement& classElement, const DataElementNode& parentDataElement, SerializeContext::ErrorHandler* errorHandler) const;

        DataElement         m_element; ///< Serialization data for this element.
        const ClassData* m_classData; ///< Reflected ClassData for this element.
        NodeArray           m_subElements; ///< Nodes of sub elements.
    };
    // @}

    /**
    * Storage for persistent parameters passed to a root EnumerateInstance pass.
    * EnumerateInstance is used in high frequency performance-sensitive scenarios, and this ensures
    * minimal interaction with the memory manager for things like bound functors.
    */
    struct EnumerateInstanceCallContext
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(EnumerateInstanceCallContext);
        EnumerateInstanceCallContext(const SerializeContext::BeginElemEnumCB& beginElemCB,
            const SerializeContext::EndElemEnumCB& endElemCB,
            const SerializeContext* context, unsigned int accessflags,
            SerializeContext::ErrorHandler* errorHandler);

        SerializeContext::BeginElemEnumCB m_beginElemCB;          ///< Optional callback when entering an element's hierarchy.
        SerializeContext::EndElemEnumCB m_endElemCB;            ///< Optional callback when exiting an element's hierarchy.
        unsigned int m_accessFlags;          ///< Data access flags for the enumeration, see \ref EnumerationAccessFlags.
        SerializeContext::ErrorHandler* m_errorHandler;         ///< Optional user error handler.
        const SerializeContext* m_context;              ///< Serialize context containing class reflection required for data traversal.

        IDataContainer::ElementCB m_elementCallback;      ///< Pre-bound functor computed internally to avoid allocating closures during traversal.
        SerializeContext::ErrorHandler m_defaultErrorHandler;  ///< If no custom error handler is provided, the context provides one.
    };
} // namespace AZ::Serialize

namespace AZ
{
    template <class FromT, class ToT>
    void SerializeContext::DataPatchTypeUpgrade<FromT, ToT>::Apply(AZ::SerializeContext& context, SerializeContext::DataElementNode& node) const
    {
        auto targetElementIndex = node.FindElement(m_targetFieldCRC);

        AZ_Assert(targetElementIndex >= 0, "Invalid node. Field %s is not a valid element of class %s (Version %d). Check your reflection function.", m_targetFieldName.data(), node.GetNameString(), node.GetVersion());

        if (targetElementIndex >= 0)
        {
            AZ::SerializeContext::DataElementNode& targetElement = node.GetSubElement(targetElementIndex);

            // We're replacing the target node so store the name off for use later
            const char* targetElementName = targetElement.GetNameString();

            FromT fromData;

            // Get the current value at the target node
            bool success = targetElement.GetData<FromT>(fromData);

            AZ_Assert(success, "A single node type conversion of class %s (version %d) failed on field %s. The original node exists but isn't the correct type. Check your class reflection.", node.GetNameString(), node.GetVersion(), targetElementName);

            if (success)
            {
                node.RemoveElement(targetElementIndex);

                // Apply the user's provided data converter function
                ToT toData = m_upgrade(fromData);

                // Add the converted data back into the node as a new element with the same name
                auto newIndex = node.AddElement<ToT>(context, targetElementName);
                auto& newElement = node.GetSubElement(newIndex);
                newElement.SetData(context, toData);
            }
        }
    }

    /**
    * Base class that will provide various information for a generic entry.
    *
    * The difference between Generic id and Specialized id:
    * Generic Id represents the Uuid which represents the inherited GenericClassInfo class(ex. GenericClassBasicString -> AzTypeInfo<GenericClassBasicString>::Uuid
    * Specialized Id represents the Uuid of the class that the GenericClassinfo specializes(ex. GenericClassBasicString -> AzTypeInfo<AZStd::string>::Uuid
    * By default for non-templated classes the Specialized Id is the Uuid returned by AzTypeInfo<ValueType>::Uuid.
    * For specialized classes the Specialized Id is normally made up of the concatenation of the Template Class Uuid
    * and the Template Arguments Uuids using a SHA-1.
    */
    class GenericClassInfo
    {
    public:
        GenericClassInfo()
        { }

        virtual ~GenericClassInfo() {}

        /// Return the generic class "class Data" independent from the underlaying templates
        virtual SerializeContext::ClassData* GetClassData() = 0;

        virtual size_t GetNumTemplatedArguments() = 0;

        virtual AZ::TypeId GetTemplatedTypeId(size_t element) = 0;

        /// By default returns AzTypeInfo<ValueType>::Uuid
        virtual AZ::TypeId GetSpecializedTypeId() const = 0;

        /// Return the generic Type Id associated with the GenericClassInfo
        virtual AZ::TypeId GetGenericTypeId() const { return GetSpecializedTypeId(); }

        /// Register the generic class info using the SerializeContext
        virtual void Reflect(SerializeContext*) = 0;

        /// Returns true if the generic class can store the supplied type
        virtual bool CanStoreType(const Uuid& typeId) const { return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || GetLegacySpecializedTypeId() == typeId; }

        /// Returns the legacy specialized type id which removed the pointer types from templates when calculating type ids.
        /// i.e Typeids for AZStd::vector<AZ::Entity> and AZStd::vector<AZ::Entity*> are the same for legacy ids
        virtual AZ::TypeId GetLegacySpecializedTypeId() const { return GetSpecializedTypeId(); }
    };

    /**
     * When it comes down to template classes you have 2 choices for serialization.
     * You can register the fully specialized type (like MyClass<int>) and use it this
     * way. This is the most flexible and recommended way.
     * But often when you use template containers, pool, helpers in a big project...
     * it might be tricky to preregister all template specializations and make sure
     * there are no duplications.
     * This is why we allow to specialize a template and by doing so providing a generic
     * type, that will apply to all specializations, but it will be used on template use
     * (registered with the "Field" function). While this works fine, it's a little limiting
     * as it relies that on the fact that each class field should have unique name (which is
     * required by the system, to properly identify an element anyway). At the moment
     */
    template<class ValueType>
    struct SerializeGenericTypeInfo
    {
        // Provides a specific type alias that can be used to create GenericClassInfo of the
        // specified type. By default this is GenericClassInfo class which is abstract
        using ClassInfoType = GenericClassInfo;

        /// By default we don't have generic class info
        static GenericClassInfo* GetGenericInfo() { return nullptr; }
        /// By default just return the ValueTypeInfo
        static constexpr Uuid GetClassTypeId();
    };

    /**
    Helper structure to allow the creation of a function pointer for creating AZStd::any objects
    It takes advantage of type erasure to allow a mapping of Uuids to AZStd::any(*)() function pointers
    without needing to store the template type.
    */
    template<typename ValueType, typename = void>
    struct AnyTypeInfoConcept;

    template<typename ValueType>
    struct AnyTypeInfoConcept<ValueType, AZStd::enable_if_t<!AZStd::is_abstract<ValueType>::value && AZStd::Internal::template_is_copy_constructible<ValueType>::value>>
    {
        static AZStd::any CreateAny(SerializeContext*)
        {
            return AZStd::any(ValueType());
        }

        static AZStd::any::action_handler_for_t GetAnyActionHandler(SerializeContext*)
        {
            return AZStd::any::get_action_handler_for_t<ValueType>();
        }
    };

    template<typename ValueType>
    struct AnyTypeInfoConcept<ValueType, AZStd::enable_if_t<!AZStd::is_abstract<ValueType>::value && !AZStd::Internal::template_is_copy_constructible<ValueType>::value>>
    {
        template<typename T>
        static void* Allocate()
        {
            if constexpr (HasAZClassAllocator<T>::value)
            {
                // If the class specializes an AZ_CLASS_ALLOCATOR use that for allocating the memory
                return T::AZ_CLASS_ALLOCATOR_Allocate();
            }
            else
            {
                // Otherwise use the AZ::SystemAllocator
                return azmalloc(sizeof(T), alignof(T), AZ::SystemAllocator);
            }
        }
        template<typename T>
        static void DeAllocate(void* ptr)
        {
            if constexpr (HasAZClassAllocator<T>::value)
            {
                // If the class specializes an AZ_CLASS_ALLOCATOR use the class DeAllocator
                // static function for returning the memory back to the allocator
                return T::AZ_CLASS_ALLOCATOR_DeAllocate(ptr);
            }
            else
            {
                // Otherwise free the memory using the AZ::SystemAllocator
                azfree(ptr);
            }
        }
        // The SerializeContext CloneObject function is used to copy data between Any
        static AZStd::any::action_handler_for_t NonCopyableAnyHandler(SerializeContext* serializeContext)
        {
            return [serializeContext](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
            {
                AZ_Assert(serializeContext->FindClassData(dest->type()), "Type %s stored in any must be registered with the serialize context", AZ::AzTypeInfo<ValueType>::Name());

                switch (action)
                {
                case AZStd::any::Action::Reserve:
                {
                    if (dest->get_type_info().m_useHeap)
                    {
                        // Allocate space for object on heap
                        // This takes advantage of the fact that the pointer for an any is stored at offset 0
                        *reinterpret_cast<void**>(dest) = Allocate<ValueType>();
                    }

                    break;
                }
                case AZStd::any::Action::Construct:
                {
                    if constexpr (AZStd::is_default_constructible_v<ValueType>)
                    {
                        // Default construct the ValueType object
                        // This occurs in the case where a Copy and Move action is invoked
                        void* ptr = AZStd::any_cast<void>(dest);
                        if (ptr)
                        {
                            new (ptr) ValueType();
                        }
                    }
                    break;
                }
                case AZStd::any::Action::Copy:
                case AZStd::any::Action::Move:
                {
                    // CloneObjectInplace should act as copy assignment over an already constructed object
                    // and not a copy constructor over an uninitialized object
                    serializeContext->CloneObjectInplace(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(source), source->type());
                    break;
                }

                case AZStd::any::Action::Destruct:
                {
                    // Call the destructor
                    void* ptr = AZStd::any_cast<void>(dest);
                    if (ptr)
                    {
                        reinterpret_cast<ValueType*>(ptr)->~ValueType();
                    }
                    break;
                }
                case AZStd::any::Action::Destroy:
                {
                    // Clear memory
                    if (dest->get_type_info().m_useHeap)
                    {
                        DeAllocate<ValueType>(AZStd::any_cast<void>(dest));
                    }
                    break;
                }
                }
            };
        }

        static AZStd::any::action_handler_for_t GetAnyActionHandler(SerializeContext* serializeContext)
        {
            return NonCopyableAnyHandler(serializeContext);
        }

        static AZStd::any CreateAny(SerializeContext* serializeContext)
        {
            AZStd::any::type_info typeinfo;
            typeinfo.m_id = AZ::AzTypeInfo<ValueType>::Uuid();
            typeinfo.m_handler = NonCopyableAnyHandler(serializeContext);
            typeinfo.m_isPointer = AZStd::is_pointer<ValueType>::value;
            typeinfo.m_useHeap = AZStd::GetMax(sizeof(ValueType), AZStd::alignment_of<ValueType>::value) > AZStd::Internal::ANY_SBO_BUF_SIZE;
            if constexpr (AZStd::is_default_constructible_v<ValueType>)
            {
                return serializeContext ? AZStd::any(typeinfo, AZStd::in_place_type_t<ValueType>{}) : AZStd::any();
            }
            else
            {
                return {};
            }
        }
    };

    template<typename ValueType>
    struct AnyTypeInfoConcept<ValueType, AZStd::enable_if_t<AZStd::is_abstract<ValueType>::value>>
    {
        static AZStd::any CreateAny(SerializeContext*)
        {
            return {};
        }

        static AZStd::any::action_handler_for_t GetAnyActionHandler(SerializeContext*)
        {
            return [](AZStd::any::Action, AZStd::any*, const AZStd::any*)
            {
            };
        }
    };

    /*
     * Helper class to retrieve Uuids and perform AZRtti queries.
     * This helper uses AZRtti when available, and does what it can when not.
     * It automatically resolves pointer-to-pointers to their value types.
     * When type is available, use the static functions provided by this helper
     * directly (ex: SerializeTypeInfo<T>::GetUuid()).
     * Call GetRttiHelper<T>() to retrieve an IRttiHelper interface
     * for AZRtti-enabled types while type info is still available, so it
     * can be used for queries after type info is lost.
     */
    template<typename T>
    struct SerializeTypeInfo
    {
        typedef typename AZStd::remove_pointer<T>::type ValueType;
        static Uuid GetUuid(const T* instance = nullptr)
        {
            return GetUuid(instance, typename AZStd::is_pointer<T>::type());
        }
        static Uuid GetUuid(const T* instance, const AZStd::true_type& /*is_pointer<T>*/)
        {
            return GetUuidHelper(instance ? *instance : nullptr, typename HasAZRtti<ValueType>::type());
        }
        static Uuid GetUuid(const T* instance, const AZStd::false_type& /*is_pointer<T>*/)
        {
            return GetUuidHelper(instance, typename HasAZRtti<ValueType>::type());
        }
        static const char* GetRttiTypeName(ValueType* const* instance)
        {
            return GetRttiTypeName(instance ? *instance : nullptr, typename HasAZRtti<ValueType>::type());
        }
        static const char* GetRttiTypeName(const ValueType* instance)
        {
            return GetRttiTypeName(instance, typename HasAZRtti<ValueType>::type());
        }
        static const char* GetRttiTypeName(const ValueType* instance, const AZStd::true_type& /*HasAZRtti<ValueType>*/)
        {
            return instance ? GetRttiHelper<ValueType>()->GetActualTypeName(instance) : AzTypeInfo<ValueType>::Name();
        }
        static const char* GetRttiTypeName(const ValueType* /*instance*/, const AZStd::false_type& /*!HasAZRtti<ValueType>*/)
        {
            return "NotAZRttiType";
        }

        static AZ::TypeId GetRttiTypeId(ValueType* const* instance)
        {
            return GetRttiTypeId(instance ? *instance : nullptr, typename HasAZRtti<ValueType>::type());
        }
        static AZ::TypeId GetRttiTypeId(const ValueType* instance) { return GetRttiTypeId(instance, typename HasAZRtti<ValueType>::type()); }
        static AZ::TypeId GetRttiTypeId(const ValueType* instance, const AZStd::true_type& /*HasAZRtti<ValueType>*/)
        {
            return instance ? AZ::RttiTypeId(instance) : AzTypeInfo<ValueType>::Uuid();
        }
        static AZ::TypeId GetRttiTypeId(const ValueType* /*instance*/, const AZStd::false_type& /*!HasAZRtti<ValueType>*/)
        {
            static Uuid s_nullUuid = Uuid::CreateNull();
            return s_nullUuid;
        }

        static bool IsRttiTypeOf(const Uuid& id, const AZStd::true_type& /*HasAZRtti<ValueType>*/)
        {
            return GetRttiHelper<ValueType>()->GetTypeId(id);
        }
        static bool IsRttiTypeOf(const Uuid& /*id*/, const AZStd::false_type& /*!HasAZRtti<ValueType>*/)
        {
            return false;
        }

        static void RttiEnumHierarchy(RTTI_EnumCallback callback, void* userData, const AZStd::true_type& /*HasAZRtti<ValueType>*/)
        {
            return AZ::RttiEnumHierarchy<ValueType>(callback, userData);
        }
        static void RttiEnumHierarchy(RTTI_EnumCallback /*callback*/, void* /*userData*/, const AZStd::false_type& /*!HasAZRtti<ValueType>*/)
        {
        }

        // const pointers
        static const void* RttiCast(ValueType* const* instance, const Uuid& asType)
        {
            return RttiCast(instance ? *instance : nullptr, asType, typename HasAZRtti<ValueType>::type());
        }
        static const void* RttiCast(const ValueType* instance, const Uuid& asType)
        {
            return RttiCast(instance, asType, typename HasAZRtti<ValueType>::type());
        }
        static const void* RttiCast(const ValueType* instance, const Uuid& asType, const AZStd::true_type& /*HasAZRtti<ValueType>*/)
        {
            return AZ::RttiAddressOf(instance, asType);
        }
        static const void* RttiCast(const ValueType* instance, const Uuid& /*asType*/, const AZStd::false_type& /*!HasAZRtti<ValueType>*/)
        {
            return instance;
        }
        // non cost pointers
        static void* RttiCast(ValueType** instance, const Uuid& asType)
        {
            return RttiCast(instance ? *instance : nullptr, asType, typename HasAZRtti<ValueType>::type());
        }
        static void* RttiCast(ValueType* instance, const Uuid& asType)
        {
            return RttiCast(instance, asType, typename HasAZRtti<ValueType>::type());
        }
        static void* RttiCast(ValueType* instance, const Uuid& asType, const AZStd::true_type& /*HasAZRtti<ValueType>*/)
        {
            return AZ::RttiAddressOf(instance, asType);
        }
        static void* RttiCast(ValueType* instance, const Uuid& /*asType*/, const AZStd::false_type& /*!HasAZRtti<ValueType>*/)
        {
            return instance;
        }

    private:
        static AZ::Uuid GetUuidHelper(const ValueType* /* ptr */, const AZStd::false_type& /* !HasAZRtti<U>::type() */)
        {
            return SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
        }
        static AZ::Uuid GetUuidHelper(const ValueType* ptr, const AZStd::true_type& /* HasAZRtti<U>::type() */)
        {
            return ptr ? AZ::RttiTypeId(ptr) : SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Implementations
    //
    namespace Serialize
    {
        /// Default instance factory to create/destroy a classes (when a factory is not provided)
        template<class T, bool U = HasAZClassAllocator<T>::value, bool A = AZStd::is_abstract<T>::value >
        struct InstanceFactory
            : public SerializeContext::IObjectFactory
        {
            void* Create(const char* name) override
            {
                (void)name;
                return aznewex(name) T;
            }
            void Destroy(void* ptr) override
            {
                delete reinterpret_cast<T*>(ptr);
            }
        };

        /// Default instance for classes without AZ_CLASS_ALLOCATOR (can't use aznew) defined.
        template<class T>
        struct InstanceFactory<T, false, false>
            : public SerializeContext::IObjectFactory
        {
            void* Create([[maybe_unused]] const char* name) override
            {
                return new(azmalloc(sizeof(T), AZStd::alignment_of<T>::value, AZ::SystemAllocator))T;
            }
            void Destroy(void* ptr) override
            {
                reinterpret_cast<T*>(ptr)->~T();
                azfree(ptr, AZ::SystemAllocator, sizeof(T), AZStd::alignment_of<T>::value);
            }
        };

        /// Default instance for abstract classes. We can't instantiate abstract classes, but we have this function for assert!
        template<class T, bool U>
        struct InstanceFactory<T, U, true>
            : public SerializeContext::IObjectFactory
        {
            void* Create(const char* name) override
            {
                (void)name;
                AZ_Assert(false, "Can't instantiate abstract classs %s", name);
                return nullptr;
            }
            void Destroy(void* ptr) override
            {
                delete reinterpret_cast<T*>(ptr);
            }
        };
    }

    namespace SerializeInternal
    {
        template<class Derived, class Base>
        size_t GetBaseOffset()
        {
            Derived* der = reinterpret_cast<Derived*>(AZ_INVALID_POINTER);
            return reinterpret_cast<char*>(static_cast<Base*>(der)) - reinterpret_cast<char*>(der);
        }
    } // namespace SerializeInternal

    //=========================================================================
    // Cast
    // [02/08/2013]
    //=========================================================================
    template<class T>
    T SerializeContext::Cast(void* instance, const Uuid& instanceClassId) const
    {
        void* ptr = DownCast(instance, instanceClassId, SerializeTypeInfo<T>::GetUuid());
        return reinterpret_cast<T>(ptr);
    }

    //=========================================================================
    // EnumerateObject
    // [10/31/2012]
    //=========================================================================
    template<class T>
    bool SerializeContext::EnumerateObject(T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler) const
    {
        AZ_Assert(!AZStd::is_pointer<T>::value, "Cannot enumerate pointer-to-pointer as root element! This makes no sense!");

        void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        return EnumerateInstance(classPtr, classId, beginElemCB, endElemCB, accessFlags, nullptr, nullptr, errorHandler);
    }

    template<class T>
    bool SerializeContext::EnumerateObject(const T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler) const
    {
        AZ_Assert(!AZStd::is_pointer<T>::value, "Cannot enumerate pointer-to-pointer as root element! This makes no sense!");

        const void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        return EnumerateInstanceConst(classPtr, classId, beginElemCB, endElemCB, accessFlags, nullptr, nullptr, errorHandler);
    }

    //=========================================================================
    // CloneObject
    // [03/21/2014]
    //=========================================================================
    template<class T>
    T* SerializeContext::CloneObject(const T* obj)
    {
        // This function could have been called with a base type as the template parameter, when the object to be cloned is a derived type.
        // In this case, first cast to the derived type, since the pointer to the derived type might be offset from the base type due to multiple inheritance
        const void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        void* clonedObj = CloneObject(classPtr, classId);

        // Now that the actual type has been cloned, cast back to the requested type of the template parameter.
        return Cast<T*>(clonedObj, classId);
    }

    // CloneObject
    template<class T>
    void SerializeContext::CloneObjectInplace(T& dest, const T* obj)
    {
        const void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        CloneObjectInplace(&dest, classPtr, classId);
    }

    //=========================================================================
    // EnumerateDerived
    // [11/13/2012]
    //=========================================================================
    template<class T>
    inline void SerializeContext::EnumerateDerived(const TypeInfoCB& callback)
    {
        EnumerateDerived(callback, AzTypeInfo<T>::Uuid(), AzTypeInfo<T>::Uuid());
    }

    //=========================================================================
    // EnumerateBase
    // [11/13/2012]
    //=========================================================================
    template<class T>
    inline void SerializeContext::EnumerateBase(const TypeInfoCB& callback)
    {
        EnumerateBase(callback, AzTypeInfo<T>::Uuid());
    }

    constexpr char const* c_serializeBaseClassStrings[] = { "BaseClass1", "BaseClass2", "BaseClass3" };
    constexpr size_t c_serializeMaxNumBaseClasses = 3;
    static_assert(AZ_ARRAY_SIZE(c_serializeBaseClassStrings) == c_serializeMaxNumBaseClasses, "Expected matching array size for the names of serialized base classes.");

    template<class T, class BaseClass>
    void SerializeContext::AddClassData(ClassData* classData, size_t index)
    {
        ClassElement ed;
        ed.m_name = c_serializeBaseClassStrings[index];
        ed.m_nameCrc = AZ_CRC(c_serializeBaseClassStrings[index]);
        ed.m_flags = ClassElement::FLG_BASE_CLASS;
        ed.m_dataSize = sizeof(BaseClass);
        ed.m_typeId = AZ::AzTypeInfo<BaseClass>::Uuid();
        ed.m_offset = SerializeInternal::GetBaseOffset<T, BaseClass>();
        ed.m_genericClassInfo = SerializeGenericTypeInfo<BaseClass>::GetGenericInfo();
        ed.m_azRtti = GetRttiHelper<BaseClass>();
        ed.m_editData = nullptr;
        classData->m_elements.emplace_back(AZStd::move(ed));
    }

    template<class T, class... TBaseClasses>
    void SerializeContext::AddClassData(ClassData* classData)
    {
        size_t index = 0;
        // Note - if there are no base classes for T, the compiler will eat the function call in the initializer list and emit warnings for unrefed locals
        AZ_UNUSED(classData);
        AZ_UNUSED(index);
        std::initializer_list<size_t> stuff{ (AddClassData<T, TBaseClasses>(classData, index), index++)... };
        AZ_UNUSED(stuff);
    }

    //=========================================================================
    // Class
    // [10/31/2012]
    //=========================================================================
    template<class T, class... TBaseClasses>
    SerializeContext::ClassBuilder SerializeContext::Class()
    {
        return Class<T, TBaseClasses...>(&Serialize::StaticInstance<Serialize::InstanceFactory<T> >::s_instance);
    }

    //=========================================================================
    // Class
    // [10/31/2012]
    //=========================================================================
    template<class T, class... TBaseClasses>
    SerializeContext::ClassBuilder SerializeContext::Class(IObjectFactory* factory)
    {
        static_assert((AZStd::negation_v<AZStd::disjunction<AZStd::is_same<T, TBaseClasses>...> >), "You cannot reflect a type as its own base");
        static_assert(sizeof...(TBaseClasses) <= c_serializeMaxNumBaseClasses, "Only " AZ_STRINGIZE(c_serializeMaxNumBaseClasses) " base classes are supported. You can add more in c_serializeBaseClassStrings.");

        const Uuid& typeUuid = AzTypeInfo<T>::Uuid();
        const char* name = AzTypeInfo<T>::Name();

        auto deprecatedNameVisitor = [](const DeprecatedTypeNameCallback& callback)
        {
            DeprecatedTypeNameVisitor<T>(callback);
        };

        if (IsRemovingReflection())
        {
            return UnreflectClassInternal(name, typeUuid, deprecatedNameVisitor);
        }
        else
        {
            auto createAnyActionHandler = [](SerializeContext* serializeContext) -> AZStd::any::action_handler_for_t
            {
                return AnyTypeInfoConcept<T>::GetAnyActionHandler(serializeContext);
            };
            ClassBuilder builder = ReflectClassInternal(name, typeUuid,
                factory, deprecatedNameVisitor,
                GetRttiHelper<T>(), &AnyTypeInfoConcept<T>::CreateAny, AZStd::move(createAnyActionHandler));
            AddClassData<T, TBaseClasses...>(&builder.m_classData->second);

            return builder;
        }
    }

    //=========================================================================
    // ClassBuilder::Field
    // [10/31/2012]
    //=========================================================================
    template<class ClassType, class FieldType>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Field(const char* name, FieldType ClassType::* member, AZStd::initializer_list<AttributePair> attributes)
    {
        if (m_context->IsRemovingReflection())
        {
            // Delete any attributes allocated for this call.
            for (auto& attributePair : attributes)
            {
                delete attributePair.second;
            }
            return this; // we have already removed the class data for this class
        }

        using UnderlyingType = AZStd::RemoveEnumT<FieldType>;
        using ValueType = AZStd::remove_pointer_t<FieldType>;

        const char* className = AzTypeInfo<ClassType>::Name();
        const AZ::TypeId classId = AzTypeInfo<ClassType>::Uuid();
        constexpr size_t fieldSize = sizeof(FieldType);
        const size_t fieldOffset = reinterpret_cast<size_t>(&(reinterpret_cast<ClassType const volatile*>(0)->*member));
        constexpr bool fieldIsPointer = AZStd::is_pointer_v<FieldType>;
        constexpr bool fieldIsEnum = AZStd::is_enum_v<ValueType>;

        // SerializeGenericTypeInfo<ValueType>::GetClassTypeId() is needed solely because
        // the SerializeGenericTypeInfo specialization for AZ::Data::Asset<T> returns the GetAssetClassId() value
        // and not the AzTypeInfo<AZ::Data::Asset<T>>::Uuid()
        // Therefore in order to remain backwards compatible the SerializeGenericTypeInfo<ValueType>::GetClassTypeId specialization
        // is used for all cases except when the ValueType is an enum type.
        // In that case AzTypeInfo is used directly to retrieve the actual Uuid that the enum specializes
        const AZ::TypeId fieldTypeId = fieldIsEnum ? AzTypeInfo<ValueType>::Uuid() : SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
        const AZ::TypeId fieldUnderlyingTypeId = AzTypeInfo<UnderlyingType>::Uuid();
        IRttiHelper* fieldRttiHelper = GetRttiHelper<ValueType>();
        GenericClassInfo* fieldGenericClassInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
        CreateAnyFunc fieldCreateAnyFunc = &AnyTypeInfoConcept<FieldType>::CreateAny;
        return FieldImpl(attributes, className, classId, name, fieldTypeId, fieldOffset,
            fieldSize, fieldIsPointer, fieldIsEnum, fieldUnderlyingTypeId,
            fieldRttiHelper, fieldGenericClassInfo, fieldCreateAnyFunc);
    }

    /// Declare a type change between serialized versions of a field
    //=========================================================================
    // ClassBuilder::TypeChange
    // [4/2/2019]
    //=========================================================================
    template <class FromT, class ToT>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::TypeChange(AZStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion, AZStd::function<ToT(const FromT&)> upgradeFunc)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        AZ_Error("Serialize", !m_classData->second.m_serializer, "Class has a custom serializer, and can not have per-node version upgrades.");
        AZ_Error("Serialize", !m_classData->second.m_elements.empty(), "Class has no defined elements to add per-node version upgrades to.");

        if (m_classData->second.m_serializer || m_classData->second.m_elements.empty())
        {
            return this;
        }

        m_classData->second.m_dataPatchUpgrader.AddFieldUpgrade(aznew DataPatchTypeUpgrade<FromT, ToT>(fieldName, fromVersion, toVersion, upgradeFunc));

        return this;
    }

    //=========================================================================
    // ClassBuilder::FieldFromBase
    //=========================================================================
    template<class ClassType, class BaseType, class FieldType>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::FieldFromBase(const char* name, FieldType BaseType::* member)
    {
        static_assert((AZStd::is_base_of<BaseType, ClassType>::value), "Classes BaseType and ClassType are unrelated. BaseType should be base class for ClassType");

        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        AZ_Assert(!m_classData->second.m_serializer,
            "Class %s has a custom serializer, and can not have additional fields. Classes can either have a custom serializer or child fields.",
            name);

        // make sure the class have not been reflected.
        for (ClassElement& element : m_classData->second.m_elements)
        {
            if (element.m_flags & ClassElement::FLG_BASE_CLASS)
            {
                AZ_Assert(element.m_typeId != AzTypeInfo<BaseType>::Uuid(),
                    "You can not reflect %s as base class of %s with Class<%s,%s> and then reflect the some of it's fields with FieldFromBase! Either use FieldFromBase or just reflect the entire base class!",
                    AzTypeInfo<BaseType>::Name(), AzTypeInfo<ClassType>::Name(), AzTypeInfo<ClassType>::Name(), AzTypeInfo<BaseType>::Name()
                );
            }
            else
            {
                break; // base classes are sorted at the start
            }
        }

        return Field<ClassType>(name, static_cast<FieldType ClassType::*>(member));
    }

    //=========================================================================
    // ClassBuilder::Attribute
    //=========================================================================
    template <class T>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Attribute(Crc32 idCrc, T&& value)
    {
        static_assert(!std::is_lvalue_reference<T>::value || std::is_copy_constructible<T>::value, "If passing an lvalue-reference to Attribute, it must be copy constructible");
        static_assert(!std::is_rvalue_reference<T>::value || std::is_move_constructible<T>::value, "If passing an rvalue-reference to Attribute, it must be move constructible");

        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        using ContainerType = AttributeContainerType<AZStd::decay_t<T>>;

        AZ_Assert(m_currentAttributes, "Current data type doesn't support attributes!");
        if (m_currentAttributes)
        {
            m_currentAttributes->emplace_back(idCrc, aznew ContainerType(AZStd::forward<T>(value)));
        }
        return this;
    }
}

namespace AZ
{
    //=========================================================================
    // SerializeContext::RegisterGenericType<ValueType>
    //=========================================================================
    template <class ValueType>
    void SerializeContext::RegisterGenericType()
    {
        auto genericInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
        if (genericInfo)
        {
            genericInfo->Reflect(this);
        }
    }

    //=========================================================================
    // SerializeGenericTypeInfo<ValueType>::GetClassTypeId
    //=========================================================================
    template<class ValueType>
    constexpr Uuid SerializeGenericTypeInfo<ValueType>::GetClassTypeId()
    {
        // Detect the scenario when an enum type doesn't specialize AzTypeInfo
        // The underlying type Uuid is returned instead
        return AZStd::is_enum<ValueType>::value && AzTypeInfo<ValueType>::Uuid().IsNull() ? AzTypeInfo<AZStd::RemoveEnumT<ValueType>>::Uuid() : AzTypeInfo<ValueType>::Uuid();
    };

    /**
    * PerModuleGenericClassInfo tracks module specific reflections of GenericClassInfo for each serializeContext
    * registered with this module(.dll)
    */
    class SerializeContext::PerModuleGenericClassInfo final
    {
    public:
        using GenericInfoModuleMap = AZStd::unordered_map<AZ::Uuid, AZ::GenericClassInfo*>;

        ~PerModuleGenericClassInfo();

        void AddGenericClassInfo(AZ::GenericClassInfo* genericClassInfo);
        void RemoveGenericClassInfo(const AZ::TypeId& canonicalTypeId);

        void RegisterSerializeContext(AZ::SerializeContext* serializeContext);
        void UnregisterSerializeContext(AZ::SerializeContext* serializeContext);

        /// Creates GenericClassInfo and registers it with the current module if it has not already been registered
        /// Returns a pointer to the GenericClassInfo derived class that was created
        template <typename T>
        typename SerializeGenericTypeInfo<T>::ClassInfoType* CreateGenericClassInfo();

        /// Returns GenericClassInfo registered with the current module.
        template <typename T>
        AZ::GenericClassInfo* FindGenericClassInfo() const;
        AZ::GenericClassInfo* FindGenericClassInfo(const AZ::TypeId& genericTypeId) const;

        void Cleanup();

    private:
        GenericInfoModuleMap m_moduleLocalGenericClassInfos;
        using SerializeContextSet = AZStd::unordered_set<SerializeContext*>;
        SerializeContextSet m_serializeContextSet;
    };

    template<typename T>
    typename SerializeGenericTypeInfo<T>::ClassInfoType* SerializeContext::PerModuleGenericClassInfo::CreateGenericClassInfo()
    {
        using GenericClassInfoType = typename SerializeGenericTypeInfo<T>::ClassInfoType;
        static_assert(AZStd::is_base_of<AZ::GenericClassInfo, GenericClassInfoType>::value, "GenericClassInfoType must be be derived from AZ::GenericClassInfo");

        const AZ::TypeId& canonicalTypeId = AZ::AzTypeInfo<T>::Uuid();
        auto findIt = m_moduleLocalGenericClassInfos.find(canonicalTypeId);
        if (findIt != m_moduleLocalGenericClassInfos.end())
        {
            return static_cast<GenericClassInfoType*>(findIt->second);
        }

        auto genericClassInfo = azcreate(GenericClassInfoType, ());
        if (genericClassInfo)
        {
            AddGenericClassInfo(genericClassInfo);
        }
        return genericClassInfo;
    }

    template<typename T>
    AZ::GenericClassInfo* SerializeContext::PerModuleGenericClassInfo::FindGenericClassInfo() const
    {
        return FindGenericClassInfo(AZ::AzTypeInfo<T>::Uuid());
    }

    /// Creates AZ::Attribute that is allocated using the the SerializeContext PerModule allocator
    /// associated with current module
    /// @param attrValue value to store within the attribute
    /// @param ContainerType second parameter which is used for function parameter deduction
    template <typename T, typename ContainerType>
    AttributePtr CreateModuleAttribute(T&& attrValue)
    {
        void* rawMemory = AllocatorInstance<SystemAllocator>::Get().allocate(sizeof(ContainerType), alignof(ContainerType));
        new (rawMemory) ContainerType{ AZStd::forward<T>(attrValue) };
        auto attributeDeleter = [](Attribute* attribute)
        {
            attribute->~Attribute();
            AllocatorInstance<SystemAllocator>::Get().deallocate(attribute);
        };

        return AttributePtr{ static_cast<ContainerType*>(rawMemory), AZStd::move(attributeDeleter) };
    }
}   // namespace AZ

namespace AZ::Serialize
{
    //=========================================================================
    // DataElementNode::GetData
    // [10/31/2012]    bool SerializeContext::DataElementNode::GetDataHierarchy(SerializeContext& sc, T& value, ErrorHandler* errorHandler)
    template <typename T>
    bool DataElementNode::GetData(T& value, SerializeContext::ErrorHandler* errorHandler)
    {
        const Uuid& classTypeId = SerializeGenericTypeInfo<T>::GetClassTypeId();
        const Uuid& underlyingTypeId = SerializeGenericTypeInfo<AZStd::RemoveEnumT<T>>::GetClassTypeId();
        GenericClassInfo* genericInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        const bool genericClassStoresType = genericInfo && genericInfo->CanStoreType(m_element.m_id);
        const bool typeIdsMatch = classTypeId == m_element.m_id || underlyingTypeId == m_element.m_id;
        if (typeIdsMatch || genericClassStoresType)
        {
            const ClassData* classData = m_classData;

            // check generic types
            if (!classData)
            {
                if (genericInfo)
                {
                    classData = genericInfo->GetClassData();
                }
            }

            if (classData && classData->m_serializer)
            {
                if (m_element.m_dataSize == 0)
                {
                    return true;
                }
                else
                {
                    if (m_element.m_dataType == DataElement::DT_TEXT)
                    {
                        // convert to binary so we can load the data
                        AZStd::string text;
                        text.resize_no_construct(m_element.m_dataSize);
                        m_element.m_byteStream.Read(text.size(), reinterpret_cast<void*>(text.data()));
                        m_element.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        m_element.m_dataSize = classData->m_serializer->TextToData(text.c_str(), m_element.m_version, m_element.m_byteStream);
                        m_element.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        m_element.m_dataType = DataElement::DT_BINARY;
                    }

                    bool isLoaded = classData->m_serializer->Load(&value, m_element.m_byteStream, m_element.m_version, m_element.m_dataType == DataElement::DT_BINARY_BE);
                    m_element.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                    return isLoaded;
                }
            }
            else
            {
                return GetDataHierarchy(&value, classTypeId, errorHandler);
            }
        }

        if (errorHandler)
        {
            const AZStd::string errorMessage =
                AZStd::string::format("Specified class type {%s} does not match current element %s with type {%s}.",
                    classTypeId.ToString<AZStd::string>().c_str(), m_element.m_name ? m_element.m_name : "", m_element.m_id.ToString<AZStd::string>().c_str());
            errorHandler->ReportError(errorMessage.c_str());
        }

        return false;
    }

    //=========================================================================
    // DataElementNode::GetDataHierarchy
    //=========================================================================
    template <typename T>
    bool DataElementNode::GetDataHierarchy(SerializeContext&, T& value, SerializeContext::ErrorHandler* errorHandler)
    {
        return GetData<T>(value, errorHandler);
    }

    //=========================================================================
    // DataElementNode::FindSubElementAndGetData
    //=========================================================================
    template <typename T>
    bool DataElementNode::FindSubElementAndGetData(AZ::Crc32 crc, T& outValue)
    {
        if (DataElementNode* subElementNode = FindSubElement(crc))
        {
            return subElementNode->GetData<T>(outValue);
        }

        return false;
    }

    //=========================================================================
    // DataElementNode::GetData
    // [10/31/2012]
    //=========================================================================
    template<typename T>
    bool DataElementNode::GetChildData(u32 childNameCrc, T& value)
    {
        int dataElementIndex = FindElement(childNameCrc);
        if (dataElementIndex != -1)
        {
            return GetSubElement(dataElementIndex).GetData(value);
        }
        return false;
    }

    //=========================================================================
    // DataElementNode::SetData
    // [10/31/2012]
    //=========================================================================
    template<typename T>
    bool DataElementNode::SetData(SerializeContext& sc, const T& value, SerializeContext::ErrorHandler* errorHandler)
    {
        const Uuid& classTypeId = SerializeGenericTypeInfo<T>::GetClassTypeId();

        if (classTypeId == m_element.m_id)
        {
            const ClassData* classData = m_classData;

            // check generic types
            if (!classData)
            {
                GenericClassInfo* genericInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
                if (genericInfo)
                {
                    classData = genericInfo->GetClassData();
                }
            }

            if (classData && classData->m_serializer && (classData->m_doSave == nullptr || classData->m_doSave(&value)))
            {
                AZ_Assert(m_element.m_byteStream.GetCurPos() == 0, "We have to be in the beginning of the stream, as it should be for current element only!");
                m_element.m_dataSize = classData->m_serializer->Save(&value, m_element.m_byteStream);
                m_element.m_byteStream.Truncate();
                m_element.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                m_element.m_stream = &m_element.m_byteStream;

                m_element.m_dataType = DataElement::DT_BINARY;
                return true;
            }
            else
            {
                // clear the value and prepare for the new one, then set the new value
                return Convert<T>(sc) && SetDataHierarchy(sc, &value, classTypeId, errorHandler, classData);
            }
        }

        if (errorHandler)
        {
            const AZStd::string errorMessage =
                AZStd::string::format("Specified class type {%s} does not match current element %s with type {%s}.",
                    classTypeId.ToString<AZStd::string>().c_str(), m_element.m_name, m_element.m_id.ToString<AZStd::string>().c_str());
            errorHandler->ReportError(errorMessage.c_str());
        }

        return false;
    }

    //=========================================================================
    // DataElementNode::Convert
    //=========================================================================
    template<class T>
    bool DataElementNode::Convert(SerializeContext& sc)
    {
        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.m_id = SerializeGenericTypeInfo<T>::GetClassTypeId();
        m_element.m_dataSize = 0;
        m_element.m_buffer.clear();
        m_element.m_stream = &m_element.m_byteStream;

        GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        if (genericClassInfo)
        {
            m_classData = genericClassInfo->GetClassData();
        }
        else
        {
            m_classData = sc.FindClassData(m_element.m_id);
            AZ_Assert(m_classData, "You are adding element to an unregistered class!");
        }
        m_element.m_version = m_classData->m_version;

        return true;
    }

    //=========================================================================
    // DataElementNode::Convert
    //=========================================================================
    template<class T>
    bool DataElementNode::Convert(SerializeContext& sc, const char* name)
    {
        AZ_Assert(name != NULL && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

#if defined(AZ_DEBUG_BUILD)
        if (FindElement(nameCrc) != -1)
        {
            AZ_Error("Serialize", false, "We already have a class member %s!", name);
            return false;
        }
#endif // AZ_DEBUG_BUILD

        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.m_name = name;
        m_element.m_nameCrc = nameCrc;
        m_element.m_id = SerializeGenericTypeInfo<T>::GetClassTypeId();
        m_element.m_dataSize = 0;
        m_element.m_buffer.clear();
        m_element.m_stream = &m_element.m_byteStream;

        GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        if (genericClassInfo)
        {
            m_classData = genericClassInfo->GetClassData();
        }
        else
        {
            m_classData = sc.FindClassData(m_element.m_id);
            AZ_Assert(m_classData, "You are adding element to an unregistered class!");
        }
        m_element.m_version = m_classData->m_version;

        return true;
    }

    //=========================================================================
    // DataElementNode::AddElement
    // [10/31/2012]
    //=========================================================================
    template<typename T>
    int DataElementNode::AddElement(SerializeContext& sc, const char* name)
    {
        AZ_Assert(name != nullptr && strlen(name) > 0, "Empty name in an INVALID element name!");
        u32 nameCrc = Crc32(name);

#if defined(AZ_DEBUG_BUILD)
        if (FindElement(nameCrc) != -1 && !m_classData->m_container)
        {
            AZ_Error("Serialize", false, "We already have a class member %s!", name);
            return -1;
        }
#endif // AZ_DEBUG_BUILD

        DataElementNode node;
        node.m_element.m_name = name;
        node.m_element.m_nameCrc = nameCrc;
        node.m_element.m_id = SerializeGenericTypeInfo<T>::GetClassTypeId();

        GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        if (genericClassInfo)
        {
            node.m_classData = genericClassInfo->GetClassData();
        }
        else
        {
            // if we are NOT a generic container
            node.m_classData = sc.FindClassData(node.m_element.m_id);
            AZ_Assert(node.m_classData, "You are adding element to an unregistered class!");
        }

        node.m_element.m_version = node.m_classData->m_version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // DataElementNode::AddElement
    // [7/29/2016]
    //=========================================================================
    template<typename T>
    int DataElementNode::AddElementWithData(SerializeContext& sc, const char* name, const T& dataToSet)
    {
        int retVal = AddElement<T>(sc, name);
        if (retVal != -1)
        {
            m_subElements[retVal].SetData<T>(sc, dataToSet);
        }
        return retVal;
    }

    //=========================================================================
    // DataElementNode::AddElement
    // [10/31/2012]
    //=========================================================================
    template<typename T>
    int DataElementNode::ReplaceElement(SerializeContext& sc, int index, const char* name)
    {
        DataElementNode& node = m_subElements[index];

        if (node.Convert<T>(sc, name))
        {
            return index;
        }
        else
        {
            return -1;
        }
    }

    //=========================================================================
    // ClassData::ClassData
    // [10/31/2012]
    //=========================================================================
    template<class T>
    ClassData ClassData::Create(const char* name, const Uuid& typeUuid, IObjectFactory* factory, IDataSerializer* serializer, IDataContainer* container)
    {
        ClassData createdClassData = CreateImpl(name, typeUuid, factory, serializer, container, GetRttiHelper<T>(),
            [](SerializeContext* serializeContext) -> AZStd::any::action_handler_for_t
        {
            return AnyTypeInfoConcept<T>::GetAnyActionHandler(serializeContext);
        });
        return createdClassData;
    }
}

/// include AZStd containers generics
#include <AzCore/Serialization/AZStdContainers.inl>
#include <AzCore/Serialization/std/VariantReflection.inl>

// Forward declare asset serialization helper specialization
namespace AZ
{
    template<typename T>
    struct SerializeGenericTypeInfo< Data::Asset<T> >;
}

/// include implementation of SerializeContext::EnumBuilder
#include <AzCore/Serialization/SerializeContextEnum.inl>

