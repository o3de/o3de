/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzCore/Preprocessor/CodeGenBoilerplate.h>

// Behavior Context attributes (we don't need include them we can just define the attributes here, however those are some very light weight includes)
#include <AzCore/Script/ScriptContextAttributes.h>

// Use this macro to add simple string annotatations
#define AZCG_CreateAnnotation(annotation) __attribute__((annotate(annotation)))
// Use this macro to stringize arbitrary code inside of an AZCG tag into a string annotation
#define AZCG_CreateArgumentAnnotation(annotation_name, ...) AZCG_CreateAnnotation(AZ_STRINGIZE(annotation_name) "(" AZ_STRINGIZE((__VA_ARGS__)) ")")
// Use this macro to compile arbitrary code inside of an AZCG tag for error checking
#if defined(AZCG_COMPILE_ANNOTATIONS)
#define AZCG_CompileAnnotations(...) AZCG_CreateAnnotation("AZCG_IGNORE") int AZ_JOIN(compiling_annotations_lambda_, AZCG_Paste(__COUNTER__)) = ([](...){ return 0; })(__VA_ARGS__);
#else
#define AZCG_CompileAnnotations(...)
#endif

// \page tags "Code-generator tags" are use to tag you C++ code and provide that information to the codegenerator tool. When the code generator tool is defined
// AZ_CODE_GENERATOR will be defined. Depending on the settings your file can output to multiple outputs (generated) files.
// When including a generated file make sure it's the last include in your file, as we "current file" defines to automate including of the generated code as
// much as possible.
// Example:
// (MyTest.h file)
// #include <MyModule/... includes>
// ...
//
// #include "MyTest.generated.h"  // <- has the be the last include (technically the last include with generated code)
//
// class MyTestClass
// {
//   AzClass(uuid(...) ...);  //< When the code generator runs, it will use all tags to generate the code. When we run the compiler it will define a macro that pulls
//                            //< the generated code from "MyTest.generated.h"
//   ...
// };
//

#if defined(AZ_CODE_GENERATOR)
#   define AZCG_Class(ClassName, ...) AZCG_CreateArgumentAnnotation(AzClass, Class_Attribute, Identifier(ClassName), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__); AZCG_CompileAnnotations(__VA_ARGS__)
#   define AZCG_Enum(EnumName, ...) AZCG_CreateArgumentAnnotation(AzEnum, Identifier(EnumName), __VA_ARGS__) EnumName AZ_JOIN(g_azCodeGenInternal, __COUNTER__); AZCG_CompileAnnotations(__VA_ARGS__)
#   define AZCG_Property(PropertyName, ...) AZCG_CreateArgumentAnnotation(AzProperty, Identifier(PropertyName), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__); AZCG_CompileAnnotations(__VA_ARGS__)
#   define AZCG_Field(...) AZCG_CompileAnnotations(__VA_ARGS__) AZCG_CreateArgumentAnnotation(AzField, __VA_ARGS__)
#   define AZCG_Function(...) AZCG_CompileAnnotations(__VA_ARGS__) AZCG_CreateArgumentAnnotation(AzFunction, __VA_ARGS__)
#   define AZCG_EBus(EBusInterfaceName, ...) AZCG_CreateArgumentAnnotation(AzEBus, Class_Attribute, Identifier(EBusInterfaceName), __VA_ARGS__) int AZ_JOIN(g_azCodeGenInternal, __COUNTER__); AZCG_CompileAnnotations(__VA_ARGS__)
#else
#   define AZCG_Class(ClassName, ...) AZ_JOIN(AZ_GENERATED_, ClassName)
#   define AZCG_Enum(EnumName, ...)
#   define AZCG_Property(PropertyName, ...)
#   define AZCG_Field(...)
#   define AZCG_Function(...)
#   define AZCG_EBus(EBusInterfaceName, ...) AZ_JOIN(AZ_GENERATED_, EBusInterfaceName)
#endif

// Tags that might be common to multiple contexts, should be aliased into those namespaces and not used directly
namespace AzCommon
{
    struct Uuid
    {
        Uuid(const char* uuid) { (void)uuid; }
    };

    struct Name
    {
        Name(const char* name) { (void)name; }
    };

    struct Description
    {
        Description(const char* description) { (void)description; }
        Description(const char* name, const char* description) { (void)name; (void)description; }
    };

    namespace Attributes
    {
        struct AutoExpand
        {
            AutoExpand(bool autoExpand=true) { (void)autoExpand; }
        };

        struct ChangeNotify
        {
            using ChangeNotifyCallback = AZ::u32();

            template <typename T>
            ChangeNotify(AZ::u32(T::* callback)() ) { (void)callback; }
            ChangeNotify(const char* action) { (void)action; }
            ChangeNotify(const AZ::Crc32& actionCrc) { (void)actionCrc; }
            ChangeNotify(ChangeNotifyCallback callback) { (void)callback; }
        };

        struct DescriptionTextOverride
        {
            using DescriptionGetter = const AZStd::string&();

            DescriptionTextOverride(const char* description) { (void)description; }
            DescriptionTextOverride(DescriptionGetter getter) { (void)getter; }
        };

        struct NameLabelOverride
        {
            using NameLabelGetter = const AZStd::string&();

            NameLabelOverride(const char* name) { (void)name; }
            NameLabelOverride(NameLabelGetter getter) { (void)getter; }
        };

        struct Visibility
        {
            Visibility(const char* visibility) { (void)visibility; }
            Visibility(const class AZ::Crc32& visibilityCrc) { (void)visibilityCrc; }

            template <class Function>
            Visibility(Function getter) { (void)getter; }
        };

        struct Min
        {
            Min(float min) { (void)min; }
            Min(int min) { (void)min; }
        };

        struct Max
        {
            Max(float max) { (void)max; }
            Max(int max) { (void)max; }
        };

    }

    namespace Enum
    {
        struct Value
        {
            template <class Enum>
            Value(Enum value, const char* name) { (void)value; (void)name; }
        };
    }
}

namespace AzClass
{
    using AzCommon::Uuid;
    using AzCommon::Name;
    using AzCommon::Description;

    /**
     * Allows you to define a custom reflection function that we will called when the 
     * class reflect function is called. The signature is the same as any Reflect function.
     */
    struct CustomReflection
    {
        using ReflectionFunction = void(*)(AZ::ReflectContext*);
        CustomReflection(ReflectionFunction context) { (void)context; }
    };

    /**
     * You can provide a list of classes that are NOT defined in this .h/.cpp file
     * which should be reflected as part of this class reflection (e.g. support EBus interfaces).
     * Classes that are part of the same .h and .cpp will be automatically added to Component::Reflect
     * function. It's expected that provided class will have a "static void Reflect(AZ::ReflectContext*)" present (either manually or via codegen)
     */
    struct AdditionalReflection
    {
        template<class ClassToReflect>
        AdditionalReflection() {};
    };

    template <class Base, class ...Bases>
    struct Rtti
    {
        Rtti() = default;
    };

    struct Serialize
    {
        Serialize() = default;

        template <class Parent, class ...Parents>
        struct Base 
        {
            Base() = default;
        };
        
        struct OptIn
        {
            OptIn() = default;
        };
    };

    struct Version
    {
        using ConverterFunction = bool(*)(class AZ::SerializeContext& context, class AZ::SerializeContext::DataElementNode& classElement);
        Version(unsigned int version) { (void)version; }
        Version(unsigned int version, ConverterFunction converter) { (void)version; (void)converter; }
    };

    template <class ClassAllocator>
    struct Allocator
    {
        Allocator() = default;
    };

    struct PersistentId
    {
        using PersistentIdFunction = AZ::u64(*)(const void*);
        PersistentId(PersistentIdFunction getter) { (void)getter; }
    };

    template <class SerializerType>
    struct Serializer
    {
        Serializer() = default;
    };

    template <class EventHandlerType>
    struct EventHandler
    {
        EventHandler() = default;
    };

    namespace Edit
    {
        using AzCommon::Attributes::AutoExpand;
        using AzCommon::Attributes::ChangeNotify;
        using AzCommon::Attributes::NameLabelOverride;
        using AzCommon::Attributes::DescriptionTextOverride;
        using AzCommon::Attributes::Visibility;

        struct AddableByUser
        {
            AddableByUser(bool isAddable = true) { (void)isAddable; }
        };

        struct AppearsInAddComponentMenu
        {
            AppearsInAddComponentMenu(const char* category) { (void)category; }
            AppearsInAddComponentMenu(const AZ::Crc32& categoryCrc) { (void)categoryCrc; }
        };

        struct Category
        {
            Category(const char* category) { (void)category; }
        };

        struct HelpPageURL
        {
            HelpPageURL(const char* url) { (void)url; }
            template <class Function>
            HelpPageURL(Function getter) { (void)getter; }
        };
        
        struct HideIcon
        {
            HideIcon(bool isHidden=true) { (void)isHidden; }
        };

        struct Icon
        {
            Icon(const char* pathToIcon) { (void)pathToIcon; }
        };

        template <class AssetType>
        struct PrimaryAssetType
        {
            PrimaryAssetType() = default;
        };

        struct ViewportIcon
        {
            ViewportIcon(const char* pathToIcon) { (void)pathToIcon; }
        };
    }

    /**
     * Allows you to define custom class name that is different from the class actual name 
     * (otherwise you can just skip that parameter)
     */
    struct BehaviorName
    {
        BehaviorName(const char* name) { (void)name; }
    };

    /**
     * Behavior context attributes for the class.
     */
    namespace BehaviorAttributes
    {
        using namespace AZ::Script::Attributes; // Add custom implementation to support auto-complete
    };

    /**
     * List of BusTypes (behavior type names reflect in EBus<Bus>("BusName"), which can be different than the C++ name)
     * that are associated with that class. That list is used by certain applications like ScriptCanvas or Animation System
     * to manipulate the object runtime state.
     */ 
    struct BehaviorRequestBus
    {
        BehaviorRequestBus(std::initializer_list<const char*> requestBusses) { (void)requestBusses; }
    };

    /**
     * Same requirements as \ref BehaviorRequestBus, but for notification bus. Refer to the Component EBus documentation
     * for the RequestBus/NotificationBus code pattern.
     */
    struct BehaviorNotificationBus
    {
        BehaviorNotificationBus(std::initializer_list<const char*> notificationBusses) { (void)notificationBusses; }
    };
}

namespace AzComponent
{
    struct Component
    {
        Component() = default;
    };

    struct Provides
    {
        Provides(std::initializer_list<const char*> providedServices) { (void)providedServices; }
    };

    struct Requires
    {
        Requires(std::initializer_list<const char*> requiredServices) { (void)requiredServices; }
    };

    struct Dependent
    {
        Dependent(std::initializer_list<const char*> dependentServices) { (void)dependentServices; }
    };

    struct Incompatible
    {
        Incompatible(std::initializer_list<const char*> incompatibleServices) { (void)incompatibleServices; }
    };
}

namespace AzField
{
    using AzCommon::Name;
    using AzCommon::Description;

    struct Serialize
    {
        Serialize(bool serialize = true) { (void)serialize; }

        struct Name
        {
            Name(const char* name) { (void)name; }
        };
    };

    struct Edit
    {
        Edit(bool edit = true) { (void)edit; }
    };

    struct UIHandler
    {
        UIHandler(const AZ::Crc32& uiHandlerCrc) { (void)uiHandlerCrc; }
        UIHandler(const char* uiHandler) { (void)uiHandler; }
    };

    struct ElementUIHandler
    {
        ElementUIHandler(const AZ::Crc32& uiHandlerCrc) { (void)uiHandlerCrc; }
        ElementUIHandler(const char* uiHandler) { (void)uiHandler; }
    };

    struct Group
    {
        Group(const char* name) { (void)name; }
    };

    namespace Attributes // Those are Edit attributes and they should be moved under
    {
        using AzCommon::Attributes::AutoExpand;
        using AzCommon::Attributes::ChangeNotify;
        using AzCommon::Attributes::NameLabelOverride;
        using AzCommon::Attributes::DescriptionTextOverride;
        using AzCommon::Attributes::Visibility;

        struct ButtonText
        {
            ButtonText(const char* text) { (void)text; }
        };

        struct ContainerCanBeModified
        {
            ContainerCanBeModified(bool isMutable=true) { (void)isMutable; }
        };

        struct EnumValues
        {
            using Value = AzCommon::Enum::Value;

            EnumValues(std::initializer_list<const char*> values) { (void)values; }

            EnumValues(std::initializer_list<Value> valuesAndDisplayNames) { (void)valuesAndDisplayNames; }
        };

        struct IncompatibleService
        {
            IncompatibleService(const char* serviceName) { (void)serviceName; }
        };

        struct Min
        {
            Min(int) {}
            Min(float) {}
            Min(double) {}
        };

        struct Max
        {
            Max(int) {}
            Max(float) {}
            Max(double) {}
        };

        struct ReadOnly
        {
            ReadOnly(bool readOnly=true) { (void)readOnly; }
        };

        struct RequiredService
        {
            RequiredService(const char* serviceName) { (void)serviceName; }
        };

        struct Step
        {
            Step(int) {}
            Step(float) {}
            Step(double) {}
        };

        struct StringList
        {
            template <class T>
            StringList(AZStd::vector<AZStd::string> T::* vectorOfStrings) { (void)vectorOfStrings; }

            template <class Function>
            StringList(Function getter) { (void)getter; }
        };

        struct Suffix
        {
            Suffix(const char* suffix) { (void)suffix; }
        };
    }

    namespace ElementAttributes
    {
        using AzCommon::Attributes::AutoExpand;
        using AzCommon::Attributes::ChangeNotify;
        using AzCommon::Attributes::NameLabelOverride;
        using AzCommon::Attributes::DescriptionTextOverride;
        using AzCommon::Attributes::Visibility;

        using Attributes::ButtonText;
        using Attributes::ContainerCanBeModified;
        using Attributes::EnumValues;
        using Attributes::IncompatibleService;
        using Attributes::Min;
        using Attributes::Max;
        using Attributes::ReadOnly;
        using Attributes::RequiredService;
        using Attributes::Step;
        using Attributes::StringList;
        using Attributes::Suffix;
    }

    /**
     * \ref AZ::BehaviorContext codegen support. We limit the support for most common features,
     * otherwise if you need advanced support you can use the custom reflection method.
     */ 
    struct Behavior
    {
        // By default we use the field name as is in C++, however you can provide custom name. 
        Behavior(const char* customName = nullptr) { (void)customName; }
    };

    struct BehaviorReadOnly
    {
        // By default we use the field name as is in C++, however you can provide custom name. 
        BehaviorReadOnly(const char* customName = nullptr) { (void)customName; }
    };

    namespace BehaviorAttributes
    {
        using namespace AZ::Script::Attributes; // Add custom implementation to support auto-complete
    };
}

namespace AzProperty
{
    /**
    * This class reflects a property that doesn't have to have actual member variable behind it,
    * but provides a "named variable" for a system to Get and/or Set actions
    */
    struct Behavior
    {
        template<class Getter, class Setter>
        Behavior(Getter getter, Setter setter) { (void)getter; (void)setter; }
    };

    /**
    * This class reflects a property that doesn't have to have actual variable behind it,
    * or you want to perform addition actions on Get
    */
    struct BehaviorReadOnly
    {
        template<class Getter>
        BehaviorReadOnly(Getter getter) { (void)getter; }
    };

    namespace BehaviorAttributes
    {
        using namespace AZ::Script::Attributes; // Add custom implementation to support auto-complete
    };
}

namespace AzEnum
{
    using AzCommon::Uuid;
    using AzCommon::Name;
    using AzCommon::Description;

    using AzCommon::Enum::Value;

    struct Values
    {
        Values(std::initializer_list<Value> values) { (void)values; }
    };

    /**
    * Enable behavior reflection for this Enum, since we don't support namespaces for classes and enums
    * you can provide optional custom name to disambiguate, otherwise the non fully quallified enum name will be used.
    */
    struct Behavior
    {
        // By default we use the field name as is in C++, however you can provide custom name. 
        Behavior(const char* customName = nullptr) { (void)customName; }
    };

    namespace BehaviorAttributes
    {
        using namespace AZ::Script::Attributes; // Add custom implementation to support auto-complete
    };
}

namespace AzFunction
{
    using AzCommon::Name;

    /**
    * Reflection this function in the BehaviorContext. You can optionally provide custom name as behavior context
    * doesn't support function override as C++, all function names for the scope must be unique.
    */
    struct Behavior
    {
        // By default we use the field name as is in C++, however you can provide custom name. 
        // This is actually required when you have methods with the same name and different arguments as the 
        // script environments don't differentiate based on argument types.
        Behavior(const char* customName = nullptr) { (void)customName; }
    };

    /**
     * When used in EBus context it's used to define custom name.
     */
    struct BehaviorEventName
    {
        BehaviorEventName(const char* eventName) { (void)eventName; }
    };

    /**
    * Use in EBus event context. Event will name will be excluded from BehaviorContext reflection (both to be called and handled),
    * for more specific control use custom reflection.
    */
    struct BehaviorExclude
    {
        BehaviorExclude() = default;
    };

    namespace BehaviorAttributes
    {
        using namespace AZ::Script::Attributes; // Add custom implementation to support auto-complete
    };
}

namespace AzEBus
{
    /**
     * By default the EBus interfaces contain the traits that configure the EBus (currently not part of the codegen as it's line to line the same),
     * however if you have 3RD party interfaces or prefer to keep the traits external, provide the fully qualified class name here.
     */
    struct BusTraits
    {
        template<class Traits>
        BusTraits(Traits traits) { (void)traits; }
    };


    /**
    * Enabled behavior reflection for the EBus. Contrary to the class reflection which is inclusive (you are expected to mark every function that you 
    * want to reflect to behavior context), when you tag an EBus all events are considered for reflection (as it's the common case), you will 
    * need to tag events that you do not want to reflect \ref AzEBusEvent::BehaviorExclude
    */
    struct Behavior
    {
        // By default we use the EBusName as is in C++, however you can provide custom name. 
        Behavior(const char* customName = nullptr) { (void)customName; }
    };

    /**
     * Code gen will generate the EBus event handler for behavior context, all events will be included 
     * and forwards. If you need to modify that behavior write the class yourself and pass either reflect it 
     * yourself or use the EventHandler tag to use it as part of codegen.
     */
    struct BehaviorAutoEventHandler
    {
        BehaviorAutoEventHandler(const char* classTypeUUID) { (void)classTypeUUID; }
    };

    /**
     * Provide a custom class to be reflected as EBus behavior context event handler.
     */
    struct BehaviorEventHandler
    {
        template<class ClassName>
        BehaviorEventHandler(ClassName handler) { (void)handler; }
    };

    /**
    * Attributes you can define for bus.
    */
    namespace BehaviorAttributes
    {
        using namespace AZ::Script::Attributes; // Add custom implementation to support auto-complete
    };
}
