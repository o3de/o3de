/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/** @file
 * Header file for the Component base class.
 * In Open 3D Engine's component entity system, each component defines a discrete  
 * feature that can be attached to an entity.
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/NamedEntityId.h>

#include <AzCore/RTTI/ReflectContext.h>

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h> // Used as the allocator for most components.
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ
{
    class Entity;
    class ComponentDescriptor;

    typedef AZ::u32 ComponentServiceType;       ///< ID of a user-defined component service. The system uses it to build a dependency tree.
    using ImmutableEntityVector = AZStd::vector<AZ::Entity const *>;

    using ComponentTypeList = AZStd::vector<Uuid>; ///< List of Component class type IDs.
    using ComponentValidationResult = AZ::Outcome<void, AZStd::string>;

    /**
     * Base class for all components. 
     */
    class Component
    {
        friend class Entity;
    public:

        /**
         * Adds run-time type information to the component.
         */
        AZ_RTTI(AZ::Component, "{EDFCB2CF-F75D-43BE-B26B-F35821B29247}");

        /**
         * Initializes a component's internals.
         * A component's constructor should initialize the component's variables only. 
         * Because the component is not active yet, it should not connect to message buses, 
         * send messages, and so on. Similarly, the component's constructor should not 
         * attempt to cache pointers or data from other components on the same entity 
         * because those components can be added or removed at any moment. To process 
         * and initialize all resources that make a component ready to operate, use Init().
         */
        Component();

        /**
         * Destroys a component.
         * The system always calls a component's Deactivate() function before destroying it.
         */
        virtual ~Component();

        /**
         * Returns a pointer to the entity.
         * If the component is not attached to any entity, this function returns a null pointer.
         * In that case, the component is in the default state (not activated). However, 
         * except in the case of tools, you typically should not use this function. It is a best 
         * practice to access other components through EBuses instead of accessing them directly. 
         * For more information, see the 
         * <a href="http://docs.aws.amazon.com/lumberyard/latest/developerguide/component-entity-system-pg-intro.html">Programmer's Guide to Entities and Components</a> 
         * in the Open 3D Engine Developer Guide.
         * @return A pointer to the entity. If the component is not attached to any entity,
         * the return value is a null pointer.
         */
        Entity* GetEntity() const       { return m_entity; }

        /**
         * Returns the entity ID if the component is attached to an entity.
         * If the component is not attached to any entity, this function asserts. 
         * As a safeguard, make sure that GetEntity()!=nullptr.
         * @return The ID of the entity that contains the component.
         */
        EntityId GetEntityId() const;

        /**
        * Returns the NamedEntityId if the component is attached to an entity.
        * If the component is not attached to any entity, this function asserts.
        * As a safeguard, make sure that GetEntity()!=nullptr.
        * @return The ID of the entity that contains the component.
        */
        NamedEntityId GetNamedEntityId() const;

        /**
         * Returns the component ID, which is valid only when the component is attached to an entity.
         * If the component is not attached to any entity, the return value is 0.
         * As a safeguard, make sure that GetEntity()!=nullptr.
         * @return The ID of the component. If the component is attached to any entity, 
         * the return value is 0.
         */
        ComponentId GetId() const       { return m_id; }

        /**
        * Returns the type ID
        * Can be overridden for components that wrap other components, to provide a punch through
        * to the wrapped component's ID.
        * @return The type ID of the component.
        */
        virtual const TypeId& GetUnderlyingComponentType() const { return RTTI_GetType(); }

        /**
         * Sets the component ID.
         * This function is for internal use only.
         * @param id The ID to assign to the component.
         */
        void SetId(const ComponentId& id)   { m_id = id; }

        /**
        * Override to conduct per-component or per-slice validation logic during slice asset processing.
        * @param sliceEntities All entities that belong to the slice that the entity with this component is on.
        * @param platformTags List of platforms supplied during slice asset processing.
        */
        virtual ComponentValidationResult ValidateComponentRequirements(const ImmutableEntityVector& /*sliceEntities*/,
            const AZStd::unordered_set<AZ::Crc32>& /*platformTags*/) const { return AZ::Success(); }

        /**
         * Set the component's configuration.
         * A component cannot be configured while it is activated.
         * A component must implement the ReadInConfig() function for this to have an effect.
         * @param config The component will set its properties based on this configuration.
         * The configuration class must be of the appropriate type for this component.
         * For example, use a TransformConfig with a TransformComponent.
         */
        bool SetConfiguration(const AZ::ComponentConfig& config);

        /**
         * Get a component's configuration.
         * A component must implement the WriteOutConfig() function for this to have an effect.
         * @param outConfig[out] The component will copy its properties into this configuration class.
         * The configuration class must be of the appropriate type for this component.
         * For example, use a TransformConfig with a TransformComponent.
         */
        bool GetConfiguration(AZ::ComponentConfig& outConfig) const;

    protected:
        /**
         * Initializes a component's resources.
         * (Optional) Override this function to initialize resources that the component needs.
         * The system calls this function once for each entity that owns the component. Although the
         * Init() function initializes the component, the component is not active until the system
         * calls the component's Activate() function. We recommend that you minimize the component's
         * CPU and memory overhead when the component is inactive.
         */
        virtual void Init() {}

        /**
         * Puts the component into an active state.
         * The system calls this function once during activation of each entity that owns the
         * component. You must override this function. The system calls a component's Activate()
         * function only if all services and components that the component depends on are present
         * and active. Use GetProvidedServices and GetDependentServices to specify these dependencies.
         */
        virtual void Activate() = 0;

        /**
         * Deactivates the component.
         * The system calls this function when the owning entity is being deactivated. You must
         * override this function. As a best practice, ensure that this function returns the component
         * to a minimal footprint. The order of deactivation is the reverse of activation, so your
         * component is deactivated before the components it depends on.
         *
         * The system always calls the component's Deactivate() function before destroying the component.
         * However, deactivation is not always followed by the destruction of the component. An entity and
         * its components can be deactivated and reactivated without being destroyed. Ensure that your
         * Deactivate() implementation can handle this scenario.
         */
        virtual void Deactivate() = 0;

        /**
         * Read properties from the configuration class into the component.
         * Overriding this function allows your component to be configured at runtime.
         * See AZ::ComponentConfig for more details.
         * This function cannot be invoked while the component is activated.
         * 
         * @code{.cpp}
         * // sample implementation
         * bool ReadInConfig(const ComponentConfig* baseConfig) override
         * {
         *     if (auto config = azrtti_cast<const MyConfig*>(baseConfig))
         *     {
         *         m_propertyA = config->m_propertyA;
         *         m_propertyB = config->m_propertyB
         *         return true;
         *     }
         *     return false;
         * }
         * @endcode
         */
        virtual bool ReadInConfig(const ComponentConfig* baseConfig);

        /**
         * Write properties from the component into the configuration class.
         * Overriding this function allows your component's configuration to be queried at runtime.
         * See AZ::ComponentConfig for more details.
         *
         * @code{.cpp}
         * // sample implementation
         * bool WriteOutConfig(ComponentConfig* outBaseConfig) const override
         * {
         *     if (auto config = azrtti_cast<MyConfig*>(outBaseConfig))
         *     {
         *         config->m_propertyA = m_propertyA;
         *         config->m_propertyB = m_propertyB;
         *         return true;
         *     }
         *     return false;
         * }
         * @endcode
         */
        virtual bool WriteOutConfig(ComponentConfig* outBaseConfig) const;

        /**
         * Sets the current entity.
         * This function is called by the entity.
         * @param entity The current entity.
         */
        void SetEntity(Entity* entity);

        /**
         * Reflects the Component class.
         * This function is called by the entity.
         * @param reflection The reflection context.
         */
        static void ReflectInternal(ReflectContext* reflection);

        Entity*     m_entity;       ///< Reference to the entity that owns the component. The value is null if the component is not attached to an entity.
        ComponentId m_id;           ///< A component ID that is unique for an entity. This component ID is not unique across all entities.
    };

    /**
     * Includes the core component code required to make a component work.
     * This macro is typically included in other macros, such as AZ_COMPONENT, to 
     * create a component.
     */
    #define AZ_COMPONENT_BASE(_ComponentClass, ...)                                                                                     \
    AZ_CLASS_ALLOCATOR(_ComponentClass, AZ::SystemAllocator, 0)                                                                         \
    friend class AZ::HasComponentReflect<_ComponentClass>;                                                                              \
    friend class AZ::HasComponentProvidedServices<_ComponentClass>;                                                                     \
    friend class AZ::HasComponentDependentServices<_ComponentClass>;                                                                    \
    friend class AZ::HasComponentRequiredServices<_ComponentClass>;                                                                     \
    friend class AZ::HasComponentIncompatibleServices<_ComponentClass>;                                                                 \
    static AZ::ComponentDescriptor* CreateDescriptor()                                                                                  \
    {                                                                                                                                   \
            AZ::ComponentDescriptor* descriptor = nullptr;                                                                              \
            AZ::ComponentDescriptorBus::EventResult(descriptor, _ComponentClass::RTTI_Type(), &AZ::ComponentDescriptor::GetDescriptor); \
            if (descriptor)                                                                                                             \
            {                                                                                                                           \
                /* Compare strings first, then pointers.  If we compare pointers first, different strings will give the wrong error message */ \
                if (strcmp(descriptor->GetName(), _ComponentClass::RTTI_TypeName()) != 0)                                               \
                {                                                                                                                       \
                    AZ_Error("Component", false, "Two different components have the same UUID (%s), which is not allowed.\n"            \
                        "Change the UUID on one of them.\nComponent A: %s\nComponent B: %s",                                            \
                        _ComponentClass::RTTI_Type().ToString<AZStd::string>().c_str(), descriptor->GetName(), _ComponentClass::RTTI_TypeName());          \
                    return nullptr;                                                                                                     \
                }                                                                                                                       \
                if (descriptor->GetName() != _ComponentClass::RTTI_TypeName())                                                     \
                {                                                                                                                       \
                    AZ_Error("Component", false, "The same component UUID (%s) / name (%s) was registered twice.  This isn't allowed, " \
                             "it can cause lifetime management issues / crashes.\nThis situation can happen by declaring a component "  \
                             "in a header and registering it from two different Gems.\n",                                               \
                        _ComponentClass::RTTI_Type().ToString<AZStd::string>().c_str(), descriptor->GetName());                         \
                    return nullptr;                                                                                                     \
                }                                                                                                                       \
                return descriptor;                                                                                                      \
            }                                                                                                                           \
            return aznew DescriptorType;                                                                                                \
    }

    /**
     * Declares a descriptor class.  
     * Unless you are implementing very advanced internal functionality, we recommend using 
     * AZ_COMPONENT instead of this macro. This macro enables you to implement a static function 
     * in the Component class instead of writing a descriptor. It defines a CreateDescriptorFunction 
     * that you can call to register a descriptor. (Only one descriptor can exist per environment.) 
     * This macro fails silently if you implement the functions with the wrong signatures.
     */
    #define AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(_ComponentClass)         \
    friend class AZ::ComponentDescriptorDefault<_ComponentClass>;           \
    typedef AZ::ComponentDescriptorDefault<_ComponentClass> DescriptorType;

    /**
     * Declares a component with the default settings.
     * The component derives from AZ::Component, is not templated, uses AZ::SystemAllocator,
     * and so on. AZ_COMPONENT(_ComponentClass, _ComponentId, OtherBaseClases... Component) is 
     * included automatically.
     *
     * The component that this macro creates has a static function called CreateDescriptor
     * and a type called DescriptorType. Although you can delete the descriptor, keep in mind
     * that you cannot use component instances without a descriptor. This is because descriptors
     * are released when the component application closes or a module is unloaded. Descriptors
     * must have access to AZ::ComponentDescriptor::Reflect, AZ::ComponentDescriptor::GetProvidedServices,
     * and other descriptor services.
     *
     * You are not required to use the AZ_COMPONENT macro if you want to implement your own creation
     * functions by calling AZ_CLASS_ALLOCATOR, AZ_RTTI, and so on.
     */
    #define AZ_COMPONENT(_ComponentClass, ...)                  \
    AZ_RTTI(_ComponentClass, __VA_ARGS__, AZ::Component)        \
    AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(_ComponentClass)     \
    AZ_COMPONENT_BASE(_ComponentClass, __VA_ARGS__)

    /**
     * Provides an interface through which the system can get the details of a component
     * and reflect the component data to a variety of contexts.
     * If you implement a component descriptor, inherit from ComponentDescriptorHelper
     * to implement additional functionality.
     */
    class ComponentDescriptor
    {
    public:

        /**
         * The type of array that components use to specify provided, required, dependent,
         * and incompatible services.
         */
        typedef AZStd::vector<ComponentServiceType> DependencyArrayType;

        /**
        * This type of array is used by the warning
        */
        typedef AZStd::vector<AZStd::string> StringWarningArray;

         /**
          * Creates an instance of the component.
          * @return Returns a pointer to the component.
          */
        virtual Component* CreateComponent() = 0;

        /**
         * Gets the name of the component.
         * @return Returns a pointer to the name of the component.
         */
        virtual const char* GetName() const = 0;

        /**
         * Gets the ID of the component.
         * @return Returns a pointer to the component ID.
         */
        virtual const Uuid& GetUuid() const = 0;

        /**
         * Reflects component data into a variety of contexts (script, serialize, edit, and so on).
         * @param reflection A pointer to the reflection context.
         */
        virtual void Reflect(ReflectContext* reflection) const = 0;

        /**
         * Specifies the services that the component provides.
         * The system uses this information to determine when to create the component.
         * @param provided Array of provided services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        virtual void GetProvidedServices(DependencyArrayType& provided, const Component* instance) const    { (void)provided; (void)instance; }

        /**
         * Specifies the services that the component depends on, but does not require.
         * The system activates the dependent services before it activates this component.
         * It also deactivates the dependent services after it deactivates this component.
         * If a dependent service is missing before this component is activated, the system
         * does not return an error and still activates this component.
         * @param provided Array of dependent services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        virtual void GetDependentServices(DependencyArrayType& dependent, const Component* instance) const  { (void)dependent;  (void)instance; }

       /**
         * Specifies the services that the component requires.
         * The system activates the required services before it activates this component.
         * It also deactivates the required services after it deactivates this component.
         * If a required service is missing before this component is activated, the system
         * returns an error and does not activate this component.
         * @param provided Array of required services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        virtual void GetRequiredServices(DependencyArrayType& required, const Component* instance) const    { (void)required;  (void)instance; }

        /**
         * Specifies the services that the component cannot operate with.
         * For example, if two components provide a similar service and the system cannot use the services simultaneously,
         * each of those components would specify the other component as an incompatible service.
         * @param provided Array to fill with incompatible services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        virtual void GetIncompatibleServices(DependencyArrayType& incompatible, const Component* instance) const    { (void)incompatible;  (void)instance; }

        /**
         * Specifies warnings that you want in the component (will put a warning and a continue button).
         * @param warnings provided array of strings that would be the actual warnings.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        virtual void GetWarnings([[maybe_unused]] StringWarningArray& warnings, [[maybe_unused]] const Component* instance) const { }

        /**
         * Gets the current descriptor.
         * @param instance The current descriptor.
         */
        virtual ComponentDescriptor* GetDescriptor() { return this; }

        /**
         * Calls ComponentApplicationBus::UnregisterComponentDescriptor and deletes the descriptor.
         */
        virtual void ReleaseDescriptor();

        /**
         * Destroys the descriptor, but you should call ReleaseDescriptor() instead of using this function.
         */
        virtual ~ComponentDescriptor() = default;
    };

    /**
     * Describes the properties of the component descriptor event bus.
     * This bus uses AzTypeInfo::Uuid as the ID for the specific descriptor. Open 3D Engine allows only one
     * descriptor for each component type. When you call functions on the bus for a specific component
     * type, you can safely pass only one result variable because aggregating or overwriting results
     * is impossible.
     */
    struct ComponentDescriptorBusTraits
        : public EBusTraits
    {
        // We have one bus for each entity bus ID.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        // We can have only one descriptor per component type.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef Uuid BusIdType;

        using MutexType = AZStd::recursive_mutex;
    };
    typedef AZ::EBus<ComponentDescriptor, ComponentDescriptorBusTraits> ComponentDescriptorBus;

    /**
     * Helps you create a custom implementation of a descriptor.
     * For most cases we recommend using AZ_COMPONENT and ComponentDescriptorDefault instead.
     */
    template<class ComponentClass>
    class ComponentDescriptorHelper
        : public ComponentDescriptorBus::Handler
    {
    public:
        /**
         * Connects to the component descriptor bus.
         */
        ComponentDescriptorHelper()
        {
            BusConnect(AzTypeInfo<ComponentClass>::Uuid());
        }

        ~ComponentDescriptorHelper()
        {
            BusDisconnect();
        }

        /**
         * Creates an instance of the component.
         * @return Returns a pointer to the component.
         */
        Component* CreateComponent() override
        {
            return aznew ComponentClass;
        }

        /**
         * Gets the name of the component.
         * @return Returns a pointer to the name of the component.
         */
        const char* GetName() const override
        {
            return AzTypeInfo<ComponentClass>::Name();
        }

        /**
         * Gets the ID of the component.
         * @return Returns a pointer to the component ID.
         */
        const Uuid& GetUuid() const override
        {
            return AzTypeInfo<ComponentClass>::Uuid();
        }
    };

    /// @cond EXCLUDE_DOCS
    AZ_HAS_STATIC_MEMBER(ComponentReflect, Reflect, void, (ReflectContext*));
    AZ_HAS_STATIC_MEMBER(ComponentProvidedServices, GetProvidedServices, void, (ComponentDescriptor::DependencyArrayType &));
    AZ_HAS_STATIC_MEMBER(ComponentDependentServices, GetDependentServices, void, (ComponentDescriptor::DependencyArrayType &));
    AZ_HAS_STATIC_MEMBER(ComponentRequiredServices, GetRequiredServices, void, (ComponentDescriptor::DependencyArrayType &));
    AZ_HAS_STATIC_MEMBER(ComponentIncompatibleServices, GetIncompatibleServices, void, (ComponentDescriptor::DependencyArrayType &));
    /// @endcond

    /**
     * Default descriptor implementation.
     * This implementation forwards all descriptor calls to a static function inside the class.
     */
    template<class ComponentClass>
    class ComponentDescriptorDefault
        : public ComponentDescriptorHelper<ComponentClass>
    {
    public:

        /**
         * Specifies that this class should use the AZ::SystemAllocator for memory
         * management by default.
         */
        AZ_CLASS_ALLOCATOR(ComponentDescriptorDefault<ComponentClass>, SystemAllocator, 0);

        /**
         * Calls the static function AZ::ComponentDescriptor::Reflect if the user provided it.
         * @param A pointer to the reflection context.
         */
        void Reflect(ReflectContext* reflection) const override
        {
            static_assert(HasComponentReflect<ComponentClass>::value, "All components using ComponentDescriptorDefault (AZ_COMPONENT macro) should implement 'static void Reflect(ReflectContext* reflection)' function!");
            CallReflect(reflection, typename HasComponentReflect<ComponentClass>::type());
        }

        /**
         * Calls the static function AZ::ComponentDescriptor::GetProvidedServices, if the user provided it.
         * @param provided Array of provided services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided, const Component* instance) const override
        {
            (void)instance; // Not used by default because most components have static (not instance-dependent) services.
            CallProvidedServices(provided, typename HasComponentProvidedServices<ComponentClass>::type());
        }

        /**
         * Calls the static function AZ::ComponentDescriptor::GetDependentServices, if the user provided it.
         * @param provided Array of dependent services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent, const Component* instance) const override
        {
            (void)instance; // Not used by default because most components have static (not instance-dependent) services.
            CallDependentServices(dependent, typename HasComponentDependentServices<ComponentClass>::type());
        }

        /**
         * Calls the static function AZ::ComponentDescriptor::GetRequiredServices, if the user provided it.
         * @param provided Array of required services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required, const Component* instance) const override
        {
            (void)instance; // Not used by default because most components have static (not instance-dependent) services.
            CallRequiredServices(required, typename HasComponentRequiredServices<ComponentClass>::type());
        }

        /**
         * Calls the static function AZ::ComponentDescriptor::GetIncompatibleServices, if the user provided it.
         * @param provided Array of incompatible services.
         * @param instance Optional parameter with which you can refine services for each instance. This value is null if no instance exists.
         */
        void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible, const Component* instance) const override
        {
            (void)instance; // Not used by default because most components have static (not instance-dependent) services.
            CallIncompatibleServices(incompatible, typename HasComponentIncompatibleServices<ComponentClass>::type());
        }

    private:

        void CallReflect(ReflectContext* reflection, const AZStd::true_type&) const
        {
            ComponentClass::Reflect(reflection);
        }

        void CallReflect(ReflectContext*, const AZStd::false_type&) const
        {
        }

        void CallProvidedServices(ComponentDescriptor::DependencyArrayType& provided, const AZStd::true_type&) const
        {
            ComponentClass::GetProvidedServices(provided);
        }

        void CallProvidedServices(ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) const
        {
        }

        void CallDependentServices(ComponentDescriptor::DependencyArrayType& dependent, const AZStd::true_type&) const
        {
            ComponentClass::GetDependentServices(dependent);
        }

        void CallDependentServices(ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) const
        {
        }

        void CallRequiredServices(ComponentDescriptor::DependencyArrayType& required, const AZStd::true_type&) const
        {
            ComponentClass::GetRequiredServices(required);
        }

        void CallRequiredServices(ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) const
        {
        }

        void CallIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible, const AZStd::true_type&) const
        {
            ComponentClass::GetIncompatibleServices(incompatible);
        }

        void CallIncompatibleServices(ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) const
        {
        }
    };
}
