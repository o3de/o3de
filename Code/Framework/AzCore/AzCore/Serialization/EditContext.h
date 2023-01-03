/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/typetraits/is_function.h>

#include <AzCore/Math/Crc.h> // use by all functions to set the attribute id.

#include <AzCore/Serialization/EditContextConstants.inl>

namespace AZ
{
    namespace Edit
    {
        using AttributeId = AZ::AttributeId;
        using AttributePair = AZ::AttributePair;
        using AttributeArray = AZ::AttributeArray;

        using Attribute = AZ::Attribute;
        template<class T>
        using AttributeData = AZ::AttributeData<T>;
        template<class T>
        using AttributeMemberData = AZ::AttributeMemberData<T>;
        template<class F>
        using AttributeFunction = AZ::AttributeFunction<F>;
        template<class F>
        using AttributeMemberFunction = AZ::AttributeMemberFunction<F>;

        /**
         * Enumerates the serialization context and inserts component Uuids which match at least one tag in requiredTags
         * for the AZ::Edit::SystemComponentsTag attribute on the reflected class data
         */
        void GetComponentUuidsWithSystemComponentTag(
            const SerializeContext* serializeContext,
            const AZStd::vector<AZ::Crc32>& requiredTags,
            AZStd::unordered_set<AZ::Uuid>& componentUuids);

        /**
         * Returns true if the given classData's AZ::Edit::SystemComponentsTag attribute matches at least one tag in requiredTags
         * If the class data does not have the attribute, returns the value of defaultVal.
         * Note that a defaultVal of true should generally only be used to maintain legacy behavior with non-tagged AZ::Components
         */
        template <typename TagContainer>
        bool SystemComponentTagsMatchesAtLeastOneTag(
            const AZ::SerializeContext::ClassData* classData,
            const TagContainer& requiredTags,
            bool defaultVal = false);

        /**
         * Signature of the dynamic edit data provider function.
         * handlerPtr: pointer to the object whose edit data registered the handler
         * elementPtr: pointer to the sub-member of handlePtr that we are querying edit data for.
         * The function should return a pointer to the ElementData to use, or nullptr to use the
         * default one.
         */
        typedef const ElementData* DynamicEditDataProvider (const void* /*handlerPtr*/, const void* /*elementPtr*/, const Uuid& /*elementType*/);

        /**
         * Edit data is assigned to each SerializeContext::ClassBuilder::Field. You can assign all kinds of
         * generic attributes. You can have elements for class members called DataElements or Elements which define
         * attributes for the class itself called ClassElement.
         */
        struct ElementData
        {
            ElementData()
                : m_elementId(0)
                , m_description(nullptr)
                , m_name(nullptr)
                , m_serializeClassElement(nullptr)
            {}

            void ClearAttributes();

            bool IsClassElement() const   { return m_serializeClassElement == nullptr; }
            Edit::Attribute* FindAttribute(AttributeId attributeId) const;

            AttributeId         m_elementId;
            const char*         m_description = nullptr;
            const char*         m_name = nullptr;
            const char*         m_deprecatedName = nullptr;
            SerializeContext::ClassElement* m_serializeClassElement; ///< If nullptr this is class (logical) element, not physical element exists in the class
            AttributeArray      m_attributes;
        };

        /**
         * Class data is assigned to every Serialize::Class. Don't confuse m_elements with
         * ElementData. Elements contains class Elements (like groups, etc.) while the ElementData
         * contains attributes related to ehe SerializeContext::ClassBuilder::Field.
         */
        struct ClassData
        {
            ClassData()
                : m_name(nullptr)
                , m_description(nullptr)
                , m_classData(nullptr)
                , m_editDataProvider(nullptr)
            {}

            void ClearElements();
            const ElementData* FindElementData(AttributeId elementId) const;

            const char*                 m_name;
            const char*                 m_description;
            SerializeContext::ClassData* m_classData;
            DynamicEditDataProvider*    m_editDataProvider;
            AZStd::list<ElementData>    m_elements;
        };
    }

    /**
     * EditContext is bound to serialize context. It uses it for data manipulation.
     * It's role is to be an abstract way to generate and describe how a class should
     * be edited. It doesn't rely on any edit/editor/etc. code and it should not as
     * it's ABSTRACT. Please refer to your editor/edit tools to check what are the "id"
     * for the uiElements and their respective properties "id" and values.
     */
    class EditContext
    {
    public:
        /// @cond EXCLUDE_DOCS
        class ClassBuilder;
        class EnumBuilder;
        using ClassInfo = ClassBuilder; ///< @deprecated Use EditContext::ClassBuilder
        using EnumInfo = EnumBuilder; ///< @deprecated Use EditContext::EnumBuilder
        /// @endcond

        AZ_CLASS_ALLOCATOR(EditContext, SystemAllocator, 0);

        /**
         * EditContext uses serialize context to interact with data, so serialize context is
         * required.
         */
        EditContext(SerializeContext& serializeContext);
        ~EditContext();

        template<class T>
        ClassBuilder Class(const char* displayName, const char* description);

        template <class E>
        EnumBuilder Enum(const char* displayName, const char* description);

        void RemoveClassData(SerializeContext::ClassData* classData);

        const Edit::ElementData* GetEnumElementData(const AZ::Uuid& enumId) const;

    private:
        EditContext(const EditContext&);
        EditContext& operator=(const EditContext&);

        /**
         * Internal structure to maintain class information while we are describing a class.
         * User should call variety of functions to describe class features and data.
         * example:
         *  struct MyStruct {
         *      int m_data };
         *
         * expose for edit
         *  editContext->Class<MyStruct>("My structure","This structure was made to apply structure action!")->
         *      ClassElement(AZ::Edit::ClassElements::Group,"MyGroup")->
         *          Attribute("Callback",&MyStruct::IsMyGroup)->
         *      DataElement(AZ::Edit::UIHandlers::Slider,&MyStruct::m_data,"Structure data","My structure data")->
         *          Attribute(AZ::Edit::Attributes::Min,0)->
         *          Attribute(AZ::Edit::Attributes::Max,100)->
         *          Attribute(AZ::Edit::Attributes::Step,5);
         *
         *  or if you have some structure to group information or offer default values, etc. (that you know if the code and it's safe to include)
         *  you can do something like:
         *
         *  serializeContext->Class<MyStruct>("My structure","This structure was made to apply structure action!")->
         *      ClassElement(AZ::Edit::ClassElements::Group,"MyGroup")->
         *          Attribute("Callback",&MyStruct::IsMyGroup)->
         *      DataElement(AZ::Edit::UIHandlers::Slider,&MyStruct::m_data,"Structure data","My structure data")->
         *          Attribute("Params",SliderParams(0,100,5));
         *
         *  Attributes can be any value (must be copy constructed) or a function type. We do supported member functions and member data.
         *  look at the unit tests and example to see use cases.
         *
         */
    public:
        class ClassBuilder
        {
            friend EditContext;
            ClassBuilder(EditContext* context, SerializeContext::ClassData* classData, Edit::ClassData* classElement)
                : m_context(context)
                , m_classData(classData)
                , m_classElement(classElement)
                , m_editElement(nullptr)
            {
                AZ_Assert(context, "context cannot be null");
            }

            ClassBuilder()
                : m_context(nullptr)
                , m_classData(nullptr)
                , m_classElement(nullptr)
                , m_editElement(nullptr)
            {
                // The default constructor represents a no-op class builder which will cause this object to behave as a simple pass through
                // in cases where the edit context is in the process of un-reflecting
            }

            EditContext*                    m_context;
            SerializeContext::ClassData*    m_classData;
            Edit::ClassData*                m_classElement;
            Edit::ElementData*              m_editElement;

            bool IsValid() const { return m_context != nullptr; }

            Edit::ClassData* FindClassData(const Uuid& typeId);

            // If E is an enum, copy any globally reflected values into the ElementData
            template <class E>
            typename AZStd::enable_if<AZStd::is_enum<E>::value>::type
            CopyEnumValues(Edit::ElementData* ed);

            // Do nothing for non-enum types
            template <class E>
            typename AZStd::enable_if<!AZStd::is_enum<E>::value>::type
            CopyEnumValues(Edit::ElementData*) {}

        public:
            ~ClassBuilder() {}
            ClassBuilder* operator->()     { return this; }

            /**
             * Declare element with attributes that belong to the class SerializeContext::Class, this is a logical structure, you can have one or more ClassElements.
             * \uiId is the logical element ID (for instance "Group" when you want to group certain elements in this class.
             * then in each DataElement you can attach the appropriate group attribute.
             */
            ClassBuilder*  ClassElement(Crc32 elementIdCrc, const char* description);

            //! Helper method to end the current group (if any). This is shorthand for:
            //!     ->ClassElement(AZ::Edit::ClassElements::Group, "")
            ClassBuilder* EndGroup();

             /**
             * Declare element with attributes that belong to the class SerializeContext::Class, this is a logical structure, you can have one or more GroupElementToggles.
             * T must be a boolean variable that will enable and disable each DataElement attached to this structure.
             * \param description - Descriptive name of the field that will typically appear in a tooltip.
             * \param memberVariable - reference to the member variable so we can bind to serialization data.
             */
            template<class T>
            ClassBuilder* GroupElementToggle(const char* description, T memberVariable);


            /**
             * Declare element with an associated UI handler that does not represent a specific class member variable.
             * \param uiId - name of a UI handler used to display the element
             * \param description - description that will appear as the ID's label by default
             */
            ClassBuilder* UIElement(const char* uiId, const char* description);

            /**
            * Declare element with an associated UI handler that does not represent a specific class member variable.
            * \param uiId - crc32 of a UI handler used to display the element
            * \param description - description that will appear as the ID's label by default
            */
            ClassBuilder* UIElement(Crc32 uiIdCrc, const char* description);

            /**
            * Declare element with an associated UI handler that does not represent a specific class member variable.
            * \param uiId - crc32 of a UI handler used to display the element
            * \param name - descriptive name of the field.
            * \param description - description that will appear as the ID's label by default
            */
            ClassBuilder* UIElement(Crc32 uiIdCrc, const char* name, const char* description);

            /** Declare element that will handle a specific class member variable (SerializeContext::ClassBuilder::Field).
             * \param uiId - us element ID ("Int" or "Real", etc. how to edit the memberVariable)
             * \param memberVariable - reference to the member variable to we can bind to serializations data.
             * \param name - descriptive name of the field. Use this when using types in context. For example T is 'int' and name describes what it does.
             * Sometime 'T' will have edit context with enough information for name and description. In such cases use the DataElement function below.
             * \param description - detailed description that will usually appear in a tool tip.
             */
            template<class T>
            ClassBuilder* DataElement(const char* uiId, T memberVariable, const char* name, const char* description);

            /** Declare element that will handle a specific class member variable (SerializeContext::ClassBuilder::Field).
            * \param uiId - Crc32 of us element ID ("Int" or "Real", etc. how to edit the memberVariable)
            * \param memberVariable - reference to the member variable to we can bind to serializations data.
            * \param name - descriptive name of the field. Use this when using types in context. For example T is 'int' and name describes what it does.
            * Sometime 'T' will have edit context with enough information for name and description. In such cases use the DataElement function below.
            * \param description - detailed description that will usually appear in a tool tip.
            */
            template<class T>
            ClassBuilder* DataElement(Crc32 uiIdCrc, T memberVariable, const char* name, const char* description);

            /** Declare element that will handle a specific class member variable (SerializeContext::ClassBuilder::Field).
            * \param uiId - Crc32 of us element ID ("Int" or "Real", etc. how to edit the memberVariable)
            * \param memberVariable - reference to the member variable to we can bind to serializations data.
            * \param name - descriptive name of the field. Use this when using types in context. For example T is 'int' and name describes what it does.
            * \param deprecatedName - supports name deprecation when an element name needs to be changed.
            * Sometime 'T' will have edit context with enough information for name and description. In such cases use the DataElement function below.
            * \param description - detailed description that will usually appear in a tool tip.
            */
            template<class T>
            ClassBuilder* DataElement(Crc32 uiIdCrc, T memberVariable, const char* name, const char* description, const char* deprecatedName);

            /**
             * Same as above, except we will get the name and description from the edit context of the 'T' type. If 'T' doesn't have edit context
             * both name and the description will be set AzTypeInfo<T>::Name().
             * \note this function will search the context for 'T' type, it must be registered at the time of reflection, otherwise it will use the fallback
             * (AzTypeInfo<T>::Name()).
             */
            template<class T>
            ClassBuilder* DataElement(const char* uiId, T memberVariable);

            /**
            * Same as above, except we will get the name and description from the edit context of the 'T' type. If 'T' doesn't have edit context
            * both name and the description will be set AzTypeInfo<T>::Name().
            * \note this function will search the context for 'T' type, it must be registered at the time of reflection, otherwise it will use the fallback
            * (AzTypeInfo<T>::Name()).
            */
            template<class T>
            ClassBuilder* DataElement(Crc32 uiIdCrc, T memberVariable);

            /**
             * All T (attribute value) MUST be copy constructible as they are stored in internal
             * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
             * Attributes can be assigned to either classes or DataElements.
             */
            template<class T>
            ClassBuilder* Attribute(const char* id, T value);

            /**
            * All T (attribute value) MUST be copy constructible as they are stored in internal
            * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
            * Attributes can be assigned to either classes or DataElements.
            */
            template<class T>
            ClassBuilder* Attribute(Crc32 idCrc, T value);

            /**
             * Specialized attribute for defining enum values with an associated description.
             * Given the prevalence of the enum case, this prevents users from having to manually
             * create a pair<value, description> for every reflected enum value.
             * \note Do not add a bunch of these kinds of specializations. This one is generic
             * enough and common enough that it's warranted for programmer ease.
             */
            template<class T>
            ClassBuilder* EnumAttribute(T value, const char* descriptor);

            /**
             * Specialized attribute for setting properties on elements inside a container.
             * This could be used to specify a spinbox handler for all elements inside a container
             * while only having to specify it once (on the parent container).
             */
            template<class T>
            ClassBuilder* ElementAttribute(const char* id, T value);

            /**
            * Specialized attribute for setting properties on elements inside a container.
            * This could be used to specify a spinbox handler for all elements inside a container
            * while only having to specify it once (on the parent container).
            */
            template<class T>
            ClassBuilder* ElementAttribute(Crc32 idCrc, T value);

            ClassBuilder* SetDynamicEditDataProvider(Edit::DynamicEditDataProvider* pHandler);
        };

        /**
        * Internal structure to maintain class information while we are describing an enum globally.
        * User should call Value() to reflect the possible values for the enum.
        * example:
        *  struct MyStruct {
        *      SomeEnum m_data };
        *
        * expose for edit
        *  editContext->Enum<SomeEnum>("My enum","This enum was made to apply enumerated action!")->
        *      Value("SomeValue", SomeEnum::SomeValue)->
        *      Value("SomeOtherValue", SomeEnum::SomeOtherValue);
        *
        *
        */
        class EnumBuilder
        {
            friend EditContext;

            EnumBuilder(EditContext* editContext, Edit::ElementData* elementData)
                : m_context(editContext)
                , m_elementData(elementData)
            {
                AZ_Assert(editContext, "The edit context cannot be null");
            }

            EnumBuilder()
                : m_context(nullptr)
                , m_elementData(nullptr)
            {
                // The default constructor represents a no-op class builder which will cause this object to behave as a simple pass through
                // in cases where the edit context is in the process of un-reflecting
            }


            EditContext* m_context;
            Edit::ElementData* m_elementData;

            bool IsValid() const { return m_context != nullptr; }
        public:
            EnumBuilder* operator->() { return this; }

            template <class E>
            EnumBuilder* Value(const char* name, E value);
        };

        /// Analog to SerializeContext::EnumerateInstanceCallContext for enumerating an EditContext
        struct EnumerateInstanceCallContext
        {
            AZ_TYPE_INFO(EnumerateInstanceCallContext, "{FCC1DB4B-72BD-4D78-9C23-C84B91589D33}");
            EnumerateInstanceCallContext(
                const SerializeContext::BeginElemEnumCB& beginElemCB,
                const SerializeContext::EndElemEnumCB& endElemCB,
                const EditContext* context,
                unsigned int accessflags,
                SerializeContext::ErrorHandler* errorHandler);

            SerializeContext::BeginElemEnumCB m_beginElemCB; ///< Optional callback when entering an element's hierarchy.
            SerializeContext::EndElemEnumCB m_endElemCB; ///< Optional callback when exiting an element's hierarchy.
            unsigned int m_accessFlags; ///< Data access flags for the enumeration, see \ref EnumerationAccessFlags.
            SerializeContext::ErrorHandler* m_errorHandler; ///< Optional user error handler.

            SerializeContext::IDataContainer::ElementCB
                m_elementCallback; ///< Pre-bound functor computed internally to avoid allocating closures during traversal.
            SerializeContext::ErrorHandler m_defaultErrorHandler; ///< If no custom error handler is provided, the context provides one.
        };

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
        bool EnumerateInstanceConst(EnumerateInstanceCallContext* callContext, const void* ptr, const Uuid& classId, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement) const;
        bool EnumerateInstance(EnumerateInstanceCallContext* callContext, void* ptr, const Uuid& classId, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement) const;

    private:
        typedef AZStd::list<Edit::ClassData> ClassDataListType;
        typedef AZStd::unordered_map<AZ::Uuid, Edit::ElementData> EnumDataMapType;

        ClassDataListType   m_classData;
        EnumDataMapType     m_enumData;
        SerializeContext&   m_serializeContext;
    };

    namespace Internal
    {
        inline Crc32 UuidToCrc32(const AZ::Uuid& uuid)
        {
            return Crc32(uuid.begin(), uuid.end() - uuid.begin());
        }

        inline bool IsModifyingGlobalEnum(Crc32 idCrc, Edit::ElementData& ed)
        {
            if (ed.m_serializeClassElement)
            {
                const Crc32 typeCrc = UuidToCrc32(ed.m_serializeClassElement->m_typeId);
                if (ed.m_elementId == typeCrc)
                {
                    return idCrc == AZ::Edit::InternalAttributes::EnumValue || idCrc == AZ::Edit::Attributes::EnumValues;
                }
            }
            return false;
        }
    }

    //=========================================================================
    // Class
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder
    EditContext::Class(const char* displayName, const char* description)
    {
        if (m_serializeContext.IsRemovingReflection())
        {
            return EditContext::ClassBuilder();
        }
        else
        {
            // find the class data in the serialize context.
            SerializeContext::UuidToClassMap::iterator classDataIter = m_serializeContext.m_uuidMap.find(AzTypeInfo<T>::Uuid());
            SerializeContext::ClassData* serializeClassData{};
            if (classDataIter != m_serializeContext.m_uuidMap.end())
            {
                serializeClassData = &classDataIter->second;
            }
            else
            {
                auto genericClassInfo = m_serializeContext.FindGenericClassInfo(AzTypeInfo<T>::Uuid());
                if (genericClassInfo)
                {
                    serializeClassData = genericClassInfo->GetClassData();
                }
            }
            AZ_Assert(serializeClassData, "Class %s is not reflected in the serializer yet! Edit context can be set after the class is reflected!", AzTypeInfo<T>::Name());

            Edit::ClassData& editClassData = m_classData.emplace_back();
            editClassData.m_name = displayName;
            editClassData.m_description = description;
            editClassData.m_editDataProvider = nullptr;
            editClassData.m_classData = serializeClassData;
            serializeClassData->m_editData = &editClassData;
            return EditContext::ClassBuilder(this, serializeClassData, &editClassData);
        }

    }

    //=========================================================================
    // Enum
    //=========================================================================
    template <class E>
    EditContext::EnumBuilder
    EditContext::Enum(const char* displayName, const char* description)
    {
        static_assert(AZ::Internal::HasAZTypeInfo<E>::value, "Enums must have reflection type info (via AZ_TYPE_INFO_SPECIALIZE or AzTypeInfo<Enum>) to be reflected globally");
        const AZ::Uuid& enumId = azrtti_typeid<E>();
        if (m_serializeContext.IsRemovingReflection())
        {
            // If the serialize context is unreflecting, then the enum needs to be removed
            auto enumDataIt = m_enumData.find(enumId);
            AZ_Assert(enumDataIt != m_enumData.end(), "Enum %s is being unreflected but was never reflected", displayName);
            enumDataIt->second.ClearAttributes();
            m_enumData.erase(enumDataIt);
            return {};
        }

        AZ_Assert(m_enumData.find(enumId) == m_enumData.end(), "Enum %s has already been reflected to EditContext", displayName);
        Edit::ElementData& enumData = m_enumData[enumId];

        // Set the elementId to the Crc of the typeId, this indicates that it's globally reflected
        const Crc32 typeCrc = AZ::Internal::UuidToCrc32(enumId);
        enumData.m_elementId = typeCrc;
        enumData.m_name = displayName;
        enumData.m_description = description;
        return {this, &enumData};
    }

    //=========================================================================
    // ClassElement
    //=========================================================================
    inline EditContext::ClassBuilder*
    EditContext::ClassBuilder::ClassElement(Crc32 elementIdCrc, const char* description)
    {
        if (IsValid())
        {
            Edit::ElementData& ed = m_classElement->m_elements.emplace_back();
            ed.m_elementId = elementIdCrc;
            ed.m_description = description;
            m_editElement = &ed;
        }
        return this;
    }

    //=========================================================================
    // ClassElement
    //=========================================================================
    template<class T>
    inline EditContext::ClassBuilder* EditContext::ClassBuilder::GroupElementToggle(const char* name, T memberVariable)
    {
        return DataElement(AZ::Edit::ClassElements::Group, memberVariable, name, name, "");
    }

    inline EditContext::ClassBuilder*
    EditContext::ClassBuilder::EndGroup()
    {
        // Starting a new group with no description ends the current group
        return ClassElement(AZ::Edit::ClassElements::Group, "");
    }

    //=========================================================================
    // UIElement
    //=========================================================================
    inline EditContext::ClassBuilder*
    EditContext::ClassBuilder::UIElement(const char* uiId, const char* description)
    {
        return UIElement(Crc32(uiId), description);
    }

    //=========================================================================
    // UIElement
    //=========================================================================
    inline EditContext::ClassBuilder*
    EditContext::ClassBuilder::UIElement(Crc32 uiIdCrc, const char* description)
    {
        auto* classBuilder = ClassElement(AZ::Edit::ClassElements::UIElement, description)->Attribute(AZ::Edit::UIHandlers::Handler, uiIdCrc);

        if (IsValid())
        {
            classBuilder->m_editElement->m_name = classBuilder->m_editElement->m_description;
        }

        return classBuilder;
    }

    //=========================================================================
    // UIElement
    //=========================================================================
    inline EditContext::ClassBuilder*
        EditContext::ClassBuilder::UIElement(Crc32 uiIdCrc, const char* name, const char* description)
    {
        auto* classBuilder = ClassElement(AZ::Edit::ClassElements::UIElement, description)->Attribute(AZ::Edit::UIHandlers::Handler, uiIdCrc);

        if (IsValid())
        {
            classBuilder->m_editElement->m_name = name;
        }

        return classBuilder;
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
    EditContext::ClassBuilder::DataElement(const char* uiId, T memberVariable, const char* name, const char* description)
    {
        return DataElement(Crc32(uiId), memberVariable, name, description, "");
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
        EditContext::ClassBuilder::DataElement(Crc32 uiIdCrc, T memberVariable, const char* name, const char* description)
    {
        return DataElement(uiIdCrc, memberVariable, name, description, "");
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
    EditContext::ClassBuilder::DataElement(Crc32 uiIdCrc, T memberVariable, const char* name, const char* description, const char* deprecatedName)
    {
        if (!IsValid())
        {
            return this;
        }

        using ElementTypeInfo = typename SerializeInternal::ElementInfo<T>;
        AZ_Assert(m_classData->m_typeId == AzTypeInfo<typename ElementTypeInfo::ClassType>::Uuid(), "Data element (%s) belongs to a different class!", name);

        // Not really portable but works for the supported compilers
        size_t offset = reinterpret_cast<size_t>(&(reinterpret_cast<typename ElementTypeInfo::ClassType const volatile*>(0)->*memberVariable));
        //offset = or pass it to the function with offsetof(typename ElementTypeInfo::ClassType,memberVariable);

        SerializeContext::ClassElement* classElement = nullptr;
        for (size_t i = 0; i < m_classData->m_elements.size(); ++i)
        {
            SerializeContext::ClassElement* element = &m_classData->m_elements[i];
            if (element->m_offset == offset)
            {
                classElement = element;
                break;
            }
        }
        // We cannot continue past this point, we must alert the user to fix their serialization config and crash
        AZ_Assert(classElement, "Class element for editor data element reflection '%s' was NOT found in the serialize context! This member MUST be serializable to be editable!", name);

        Edit::ElementData* ed = &m_classElement->m_elements.emplace_back();

        classElement->m_editData = ed;
        m_editElement = ed;
        ed->m_elementId = uiIdCrc;
        ed->m_name = name;
        ed->m_deprecatedName = deprecatedName;
        ed->m_description = description;
        ed->m_serializeClassElement = classElement;
        return this;
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder* EditContext::ClassBuilder::DataElement(const char* uiId, T memberVariable)
    {
        if (!IsValid())
        {
            return this;
        }

        using ElementTypeInfo = typename SerializeInternal::ElementInfo<T>;
        using ElementType = AZStd::conditional_t<AZStd::is_enum_v<typename ElementTypeInfo::Type>, typename ElementTypeInfo::Type, typename ElementTypeInfo::ElementType>;
        AZ_Assert(m_classData->m_typeId == AzTypeInfo<typename ElementTypeInfo::ClassType>::Uuid(), "Data element (%s) belongs to a different class!", AzTypeInfo<typename ElementTypeInfo::ValueType>::Name());

        const SerializeContext::ClassData* classData = m_context->m_serializeContext.FindClassData(AzTypeInfo<typename ElementTypeInfo::ValueType>::Uuid());
        if (classData && classData->m_editData)
        {
            return DataElement<T>(uiId, memberVariable, classData->m_editData->m_name, classData->m_editData->m_description);
        }
        else
        {
            if constexpr (AZStd::is_enum<ElementType>::value)
            {
                if (AzTypeInfo<ElementType>::Name() != nullptr)
                {
                    auto enumIter = m_context->m_enumData.find(AzTypeInfo<ElementType>::Uuid());
                    if (enumIter != m_context->m_enumData.end())
                    {
                        return DataElement<T>(uiId, memberVariable, enumIter->second.m_name, enumIter->second.m_description);
                    }
                }
            }
        }

        const char* typeName = AzTypeInfo<typename ElementTypeInfo::ValueType>::Name();
        return DataElement<T>(uiId, memberVariable, typeName, typeName);
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder* EditContext::ClassBuilder::DataElement(Crc32 uiIdCrc, T memberVariable)
    {
        if (!IsValid())
        {
            return this;
        }

        typedef typename SerializeInternal::ElementInfo<T>  ElementTypeInfo;
        AZ_Assert(m_classData->m_typeId == AzTypeInfo<typename ElementTypeInfo::ClassType>::Uuid(), "Data element (%s) belongs to a different class!", AzTypeInfo<typename ElementTypeInfo::ValueType>::Name());
        using ElementType = AZStd::conditional_t<AZStd::is_enum_v<typename ElementTypeInfo::Type>, typename ElementTypeInfo::Type, typename ElementTypeInfo::ElementType>;

        const SerializeContext::ClassData* classData = m_context->m_serializeContext.FindClassData(AzTypeInfo<typename ElementTypeInfo::ValueType>::Uuid());
        if (classData && classData->m_editData)
        {
            return DataElement<T>(uiIdCrc, memberVariable, classData->m_editData->m_name, classData->m_editData->m_description);
        }
        else if constexpr (AZStd::is_enum<ElementType>::value)
        {
            if (AzTypeInfo<ElementType>::Name() != nullptr)
            {
                auto enumIter = m_context->m_enumData.find(AzTypeInfo<ElementType>::Uuid());
                if (enumIter != m_context->m_enumData.end())
                {
                    return DataElement<T>(uiIdCrc, memberVariable, enumIter->second.m_name, enumIter->second.m_description);
                }
            }
        }

        const char* typeName = AzTypeInfo<typename ElementTypeInfo::ValueType>::Name();
        return DataElement<T>(uiIdCrc, memberVariable, typeName, typeName);
    }

    //=========================================================================
    // SetDynamicEditDataProvider
    //=========================================================================
    inline EditContext::ClassBuilder*
    EditContext::ClassBuilder::SetDynamicEditDataProvider(Edit::DynamicEditDataProvider* pHandler)
    {
        if (!IsValid())
        {
            return this;
        }

        m_classElement->m_editDataProvider = pHandler;
        return this;
    }

    //=========================================================================
    // Attribute
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
    EditContext::ClassBuilder::Attribute(const char* id, T value)
    {
        return Attribute(Crc32(id), value);
    }

    //=========================================================================
    // Attribute
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
    EditContext::ClassBuilder::Attribute(Crc32 idCrc, T value)
    {
        if (!IsValid())
        {
            return this;
        }

        AZ_Assert(AZ::Internal::AttributeValueTypeClassChecker<T>::Check(m_classData->m_typeId, m_classData->m_azRtti), "Attribute (0x%08x) doesn't belong to '%s' class! You can't reference other classes!", idCrc, m_classData->m_name);
        using ContainerType = AttributeContainerType<T>;
        AZ_Assert(m_editElement, "You can attach attributes only to UiElements!");
        if (m_editElement)
        {
            // Detect adding an EnumValue attribute to an enum which is reflected globally
            const bool modifyingGlobalEnum = AZ::Internal::IsModifyingGlobalEnum(idCrc, *m_editElement);
            AZ_Error("EditContext", !modifyingGlobalEnum, "You cannot add enum values to an enum which is globally reflected");
            if (!modifyingGlobalEnum)
            {
                m_editElement->m_attributes.push_back(Edit::AttributePair(idCrc, aznew ContainerType(value)));
            }
        }
        return this;
    }

    namespace Edit
    {
        template<class EnumType>
        struct EnumConstant
        {
            AZ_TYPE_INFO(EnumConstant, "{4CDFEE70-7271-4B27-833B-F8F72AA64C40}");

            EnumConstant() {}
            EnumConstant(EnumType first, AZStd::string_view description)
                : m_value(static_cast<AZ::u64>(first))
                , m_description(description)
            {
            }

            // Store using a u64 under the hood so this can be safely cast to any valid enum-range value
            AZ::u64 m_value;
            AZStd::string m_description;
        };

        // Automatically generate a list of EnumConstant from AzEnumTraits
        template<typename EnumType, typename UnderlyingType = AZStd::underlying_type_t<EnumType>>
        AZStd::vector<EnumConstant<UnderlyingType>> GetEnumConstantsFromTraits()
        {
            AZStd::vector<EnumConstant<UnderlyingType>> enumValues;
            for (const auto& member : AZ::AzEnumTraits<EnumType>::Members)
            {
                enumValues.emplace_back(aznumeric_cast<UnderlyingType>(member.m_value), member.m_string.data());
            }
            return enumValues;
        }
    } // namespace Edit

    //=========================================================================
    // EnumAttribute
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
    EditContext::ClassBuilder::EnumAttribute(T value, const char* description)
    {
        if (!IsValid())
        {
            return this;
        }

        static_assert(AZStd::is_enum<T>::value, "Type passed to EnumAttribute is not an enum.");
        // If the name of the element is the same as the class name, then this is the global reflection (see EditContext::Enum<E>())
        const bool isReflectedGlobally = m_editElement->m_serializeClassElement && m_editElement->m_elementId == AZ::Internal::UuidToCrc32(m_editElement->m_serializeClassElement->m_typeId);
        AZ_Error("EditContext", !isReflectedGlobally, "You cannot add enum values to an enum which is globally reflected (while reflecting %s %s)", AzTypeInfo<T>::Name(), m_editElement->m_name);
        if (!isReflectedGlobally)
        {
            const Edit::EnumConstant<T> internalValue(value, description);
            using ContainerType = Edit::AttributeData<Edit::EnumConstant<T>>;
            AZ_Assert(m_editElement, "You can attach attributes only to UiElements!");
            if (m_editElement)
            {
                m_editElement->m_attributes.push_back(Edit::AttributePair(AZ::Edit::InternalAttributes::EnumValue, aznew ContainerType(internalValue)));
            }
        }
        return this;
    }

    //=========================================================================
    // ElementAttribute
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
    EditContext::ClassBuilder::ElementAttribute(const char* id, T value)
    {
        if (!IsValid())
        {
            return this;
        }
        AZ_Assert(AZ::Internal::AttributeValueTypeClassChecker<T>::Check(m_classData->m_typeId, m_classData->m_azRtti), "ElementAttribute (%s) doesn't belong to '%s' class! You can't reference other classes!", id, m_classData->m_name);
        return ElementAttribute(Crc32(id), value);
    }

    //=========================================================================
    // ElementAttribute
    //=========================================================================
    template<class T>
    EditContext::ClassBuilder*
    EditContext::ClassBuilder::ElementAttribute(Crc32 idCrc, T value)
    {
        if (!IsValid())
        {
            return this;
        }

        const SerializeContext::ClassData* templatedClassData = nullptr;
        AZStd::string idString = "{Unknown Type Id}";

        bool belongsToContainerType = AZ::Internal::AttributeValueTypeClassChecker<T>::Check(m_classData->m_typeId, m_classData->m_azRtti);
        bool belongsToTemplatedType = false;

        if (!belongsToContainerType)
        {
            if (m_classData->m_elements.back().m_genericClassInfo)
            {
                for (size_t argumentIndex = 0; argumentIndex < m_classData->m_elements.back().m_genericClassInfo->GetNumTemplatedArguments(); argumentIndex++)
                {
                    AZ::Uuid templatedTypeId = m_classData->m_elements.back().m_genericClassInfo->GetTemplatedTypeId(argumentIndex);
                    templatedClassData = m_context->m_serializeContext.FindClassData(templatedTypeId);

                    AZ_Assert(templatedClassData, "ElementAttribute (0x%08u) potentially references class with Uuid '%s' that hasn't been reflected yet!", idCrc, idString.c_str());

                    if (AZ::Internal::AttributeValueTypeClassChecker<T>::Check(templatedClassData->m_typeId, templatedClassData->m_azRtti))
                    {
                        belongsToTemplatedType = true;
                        templatedTypeId.ToString(idString);
                    }
                }
            }
        }

        AZ_Assert(belongsToContainerType || belongsToTemplatedType, "ElementAttribute (0x%08u) doesn't belong to '%s' or any contained templated classes! You can't reference other classes!", idCrc, m_classData->m_name);

        typedef AZStd::conditional_t<AZStd::is_member_pointer_v<T>,
            AZStd::conditional_t<AZStd::is_member_function_pointer_v<T>, Edit::AttributeMemberFunction<T>, Edit::AttributeMemberData<T> >,
            AZStd::conditional_t<AZStd::is_function_v<AZStd::remove_pointer_t<T>>, Edit::AttributeFunction<AZStd::remove_pointer_t<T>>, Edit::AttributeData<T> >
        > ContainerType;

        AZ_Assert(m_editElement, "You can attach ElementAttributes only to UiElements!");

        if (m_editElement)
        {
            // Detect adding an EnumValue attribute to an enum which is reflected globally
            const bool modifyingGlobalEnum = AZ::Internal::IsModifyingGlobalEnum(idCrc, *m_editElement);
            AZ_Error("EditContext", !modifyingGlobalEnum, "You cannot add enum values to an enum which is globally reflected");
            if (!modifyingGlobalEnum)
            {
                Edit::AttributePair attribute(idCrc, aznew ContainerType(value));
                attribute.second->m_describesChildren = true;
                attribute.second->m_childClassOwned = belongsToTemplatedType;
                m_editElement->m_attributes.push_back(attribute);
            }
        }
        return this;
    }

    template <class E>
    EditContext::EnumBuilder*
    EditContext::EnumBuilder::Value(const char* name, E value)
    {
        if (!IsValid())
        {
            return this;
        }
        static_assert(AZStd::is_enum<E>::value, "Only values that are part of an enum are valid as value attributes");
        static_assert(AZ::Internal::HasAZTypeInfo<E>::value, "Enums must have reflection type info (via AZ_TYPEINFO_SPECIALIZE or AzTypeInfo<Enum>) to be reflected globally");
        AZ_Assert(m_elementData, "Attempted to add a value attribute (%s) to a non-existent enum element data", name);
        const Edit::EnumConstant<E> internalValue(value, name);
        using ContainerType = Edit::AttributeData<Edit::EnumConstant<E>>;
        if (m_elementData)
        {
            m_elementData->m_attributes.push_back(Edit::AttributePair(AZ::Edit::InternalAttributes::EnumValue, aznew ContainerType(internalValue)));
            if (m_elementData->m_elementId == AZ::Edit::UIHandlers::Default)
            {
                m_elementData->m_elementId = AZ::Edit::UIHandlers::ComboBox;
            }
        }
        return this;
    }

}   // namespace AZ

#include <AzCore/Serialization/EditContext.inl>
