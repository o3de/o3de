/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorObjectSignals.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptPropertyWatcherBus.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/set.h>

// Ideally, the properties would be able to marshal themselves
// to avoid needing this. But until then, in order to maintain
// data integrity, need to friend class in this guy.
namespace AzFramework
{
    class ScriptPropertyMarshaler;
}

namespace AzToolsFramework
{
    namespace Components
    {
        class ScriptEditorComponent;
    }
}

namespace AZ
{
    class ReflectContext;
    class ScriptProperties
    {
    public:
        static void Reflect(AZ::ReflectContext* reflection);
    };
    
    /**
    * Base class for all script properties.
    */
    class ScriptProperty
    {
    public:
        static void UpdateScriptProperty(AZ::ScriptDataContext& sdc, int valueIndex, ScriptProperty** targetProperty);        
        
        static void Reflect(AZ::ReflectContext* reflection);
        static bool VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement);

        virtual ~ScriptProperty() {}
        AZ_RTTI(AzFramework::ScriptProperty, "{D227D737-F1ED-4FB3-A1FB-38E4985D2E7A}");

        ScriptProperty() {}
        ScriptProperty(const char* name)
            : m_id(Crc32(name))
            , m_name(name) {}

        virtual const void* GetDataAddress() const = 0;
        virtual const AZ::Uuid& GetDataTypeUuid() const = 0;

        /**
         * Test if the value at the index valueIndex is of the same type as that of the instance of ScriptProperty's subclass.
         */
        virtual bool DoesTypeMatch(AZ::ScriptDataContext& /*context*/, int /*valueIndex*/) const { return false; }

        virtual ScriptProperty* Clone(const char* name = nullptr) const = 0;

        virtual bool Write(AZ::ScriptContext& context) = 0;
        virtual bool TryRead(AZ::ScriptDataContext& context, int valueIndex) { (void)context; (void)valueIndex; return false; };
        bool TryUpdate(const AZ::ScriptProperty* scriptProperty)
        {
            bool allowUpdate = azrtti_typeid(scriptProperty) == azrtti_typeid(this);

            if (allowUpdate)
            {                
                CloneDataFrom(scriptProperty);
            }

            return allowUpdate;
        }        

        AZ::u64         m_id;
        AZStd::string   m_name;

    protected:
        virtual void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) = 0;    
    };

    // Denotes a ScriptProperty that has some functions associated with it.
    // that can modify the underlying data type, and would need to be handled
    // specially in the case of an 'in place' operation when being stored for
    // network functionality.
    class FunctionalScriptProperty
        : public ScriptProperty
    {
    public:
        AZ_RTTI(FunctionalScriptProperty, "{57D7418D-6B14-4A02-B50E-2E409D23CFC6}", ScriptProperty);

        FunctionalScriptProperty();
        FunctionalScriptProperty(const char* name);

        virtual ~FunctionalScriptProperty() = default;

        virtual void EnableInPlaceControls() = 0;
        virtual void DisableInPlaceControls() = 0;

        void AddWatcher(AZ::ScriptPropertyWatcher* scriptPropertyWatcher);
        void RemoveWatcher(AZ::ScriptPropertyWatcher* scriptPropertyWatcher);

    protected:

        virtual void OnWatcherAdded(AZ::ScriptPropertyWatcher* scriptPropertyWatcher);
        virtual void OnWatcherRemoved(AZ::ScriptPropertyWatcher* scriptPropertyWatcher);

        void SignalPropertyChanged();

        AZStd::set< AZ::ScriptPropertyWatcher* >  m_watchers;
    };

    class ScriptPropertyNil
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyNil,AZ::SystemAllocator,0);
        AZ_RTTI(AzFramework::ScriptPropertyNil, "{ACAD23F6-5E75-460E-BD77-1B477750264F}", ScriptProperty);

        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);

        ScriptPropertyNil() {}
        ScriptPropertyNil(const char* name) : ScriptProperty(name) {}

        const void* GetDataAddress() const override;
        const AZ::Uuid& GetDataTypeUuid() const override;

        ScriptPropertyNil* Clone(const char* name) const override;

        bool Write(AZ::ScriptContext& context) override;
        bool TryRead(AZ::ScriptDataContext& context,int valueIndex) override;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };

    class ScriptPropertyBoolean
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyBoolean, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::ScriptPropertyBoolean, "{EA7335F8-5B9F-4744-B805-FEF9240451BD}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);

        ScriptPropertyBoolean()
            : m_value(false)        {}
        ScriptPropertyBoolean(const char* name, bool value)
            : ScriptProperty(name)
            , m_value(value) {}

        const void* GetDataAddress() const override { return &m_value; }
        const AZ::Uuid& GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyBoolean* Clone(const char* name) const override;

        bool Write(AZ::ScriptContext& context) override;
        bool TryRead(AZ::ScriptDataContext& context, int valueIndex) override;

        bool    m_value;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };

    class ScriptPropertyNumber
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyNumber, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::ScriptPropertyNumber, "{5BCDFDEB-A75D-4E83-BB74-C45299CB9826}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);

        ScriptPropertyNumber()
            : m_value(0.0f)              {}
        ScriptPropertyNumber(const char* name, double value)
            : ScriptProperty(name)
            , m_value(value)  {}

        const void* GetDataAddress() const override { return &m_value; }
        const AZ::Uuid& GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyNumber* Clone(const char* name) const override;

        bool Write(AZ::ScriptContext& context) override;
        bool TryRead(AZ::ScriptDataContext& context, int valueIndex) override;

        double  m_value;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };

    class ScriptPropertyString
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyString, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::ScriptPropertyString, "{A0229C6D-B010-47E7-8985-EE220FC7BFAF}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);

        ScriptPropertyString() {}
        ScriptPropertyString(const char* name, const char* value)
            : ScriptProperty(name)
            , m_value(value) {}

        const void* GetDataAddress() const override { return &m_value; }
        const AZ::Uuid& GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex)  const override;

        ScriptPropertyString* Clone(const char* name = nullptr) const override;

        bool Write(AZ::ScriptContext& context) override;
        bool TryRead(AZ::ScriptDataContext& context, int valueIndex) override;

        AZStd::string   m_value;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };

    class ScriptPropertyGenericClass
        : public FunctionalScriptProperty
        , public BehaviorObjectSignals::Handler
    {        
        friend class AzFramework::ScriptPropertyMarshaler;
        friend class AzToolsFramework::Components::ScriptEditorComponent;
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyGenericClass, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::ScriptPropertyGenericClass, "{80618224-814C-44D4-A7B8-14B5A36F96ED}", FunctionalScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);        

        ScriptPropertyGenericClass();
        ScriptPropertyGenericClass(const char* name, const AZ::DynamicSerializableField& value);

        ~ScriptPropertyGenericClass() override;

        const void* GetDataAddress() const override { return m_value.m_data; }
        const AZ::Uuid& GetDataTypeUuid() const override { return m_value.m_typeId; }

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyGenericClass* Clone(const char* name = nullptr) const override;
        bool Write(AZ::ScriptContext& context) override;
        bool TryRead(AZ::ScriptDataContext& context, int valueIndex) override;

        const AZ::DynamicSerializableField& GetSerializableField() const;

        template<class T>
        const T* Get() const
        {
            return m_value.Get<T>();
        }

        template<class T>
        T* Get()
        {
            return m_value.Get<T>();
        }

        template<class T>
        void Set(T* value)
        {
            m_value.Set<T>(value);
        }

        void Set(const AZ::DynamicSerializableField& sourceField);

        void EnableInPlaceControls() override;
        void DisableInPlaceControls() override;
        
        void OnMemberMethodCalled(const BehaviorMethod* behaviorMethod) override;        

    protected:

        void OnWatcherAdded(ScriptPropertyWatcher* scriptPropertyWatcher) override;
        void OnWatcherRemoved(ScriptPropertyWatcher* scriptPropertyWatcher) override;

        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;

    private:
        void MonitorBehaviorSignals(bool enabled);

        void ReleaseCache();
        void CacheObject(AZ::ScriptContext& context);

        AZ::ScriptContext*  m_scriptContext;
        bool                m_cacheObject;
        int                 m_cachedValue;
        
        AZ::DynamicSerializableField    m_value;
    };

    class ScriptPropertyNumberArray
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyNumberArray, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::ScriptPropertyNumberArray, "{76609A01-46CA-442E-8BA6-251D529886AF}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);
        static bool IsNumberArray(AZ::ScriptDataContext& context, int valueIndex);
        static void ParseNumberArray(AZ::ScriptDataContext& numberArrayTable, AZStd::vector<double>& output);

        ScriptPropertyNumberArray()         {}
        ScriptPropertyNumberArray(const char* name)
            : ScriptProperty(name)      {}

        const void* GetDataAddress() const override { return &m_values; }
        const AZ::Uuid& GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyNumberArray* Clone(const char* name = nullptr) const override;

        bool Write(AZ::ScriptContext& context) override;

        AZStd::vector<double>   m_values;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };

    class ScriptPropertyBooleanArray
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyBooleanArray, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::ScriptPropertyBooleanArray, "{3A83958C-26C7-4A59-B6D7-A7805B0EC756}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);
        static bool IsBooleanArray(AZ::ScriptDataContext& context, int valueIndex);
        static void ParseBooleanArray(AZ::ScriptDataContext& numberArrayTable, AZStd::vector<bool>& output);

        ScriptPropertyBooleanArray()        {}
        ScriptPropertyBooleanArray(const char* name)
            : ScriptProperty(name)         {}

        const void* GetDataAddress() const override { return &m_values; }
        const AZ::Uuid& GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyBooleanArray* Clone(const char* name = nullptr) const override;

        bool Write(AZ::ScriptContext& context) override;

        AZStd::vector<bool> m_values;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };

    class ScriptPropertyStringArray
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyStringArray, AZ::SystemAllocator, 0);
        AZ_RTTI(AzFramework::ScriptPropertyStringArray, "{899993A5-D717-41BB-B89B-04A27952CA6D}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);
        static bool IsStringArray(AZ::ScriptDataContext& context, int valueIndex);
        static void ParseStringArray(AZ::ScriptDataContext& numberArrayTable, AZStd::vector<AZStd::string>& output);

        ScriptPropertyStringArray()         {}
        ScriptPropertyStringArray(const char* name)
            : ScriptProperty(name)      {}

        const void* GetDataAddress() const override { return &m_values; }
        const AZ::Uuid& GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyStringArray* Clone(const char* name = nullptr) const override;

        bool Write(AZ::ScriptContext& context) override;

        AZStd::vector<AZStd::string>    m_values;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };    

    class ScriptPropertyGenericClassArray
        : public ScriptProperty
    {
    public:
        typedef AZStd::vector<AZ::DynamicSerializableField> ValueArrayType;

        AZ_CLASS_ALLOCATOR(ScriptPropertyGenericClassArray, AZ::SystemAllocator, 0);
        AZ_RTTI(AZ::ScriptPropertyGenericClassArray, "{28E986DD-CF7C-404D-9BEE-EEE067180CD1}", ScriptProperty);        
        
        static void Reflect(AZ::ReflectContext* reflection);
        static ScriptProperty* TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name);
        static bool IsGenericClassArray(AZ::ScriptDataContext& context, int valueIndex);
        static void ParseGenericClassArray(AZ::ScriptDataContext& numberArrayTable, ValueArrayType& output);

        ScriptPropertyGenericClassArray() {}
        ScriptPropertyGenericClassArray(const char* name)
            : ScriptProperty(name)        {}
        ~ScriptPropertyGenericClassArray() override
        {
            for (size_t i = 0; i < m_values.size(); ++i)
            {
                m_values[i].DestroyData();
            }
        }
        const void* GetDataAddress() const override { return &m_values; }
        const AZ::Uuid& GetDataTypeUuid() const override;
        AZ::Uuid GetElementTypeUuid() const;

        void SetElementTypeUuid(const AZ::Uuid);

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyGenericClassArray* Clone(const char* name = nullptr) const override;

        bool Write(AZ::ScriptContext& context) override;

        ValueArrayType  m_values;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
        
    private: // static member functions

        AZ::Uuid m_elementTypeId = AZ::Uuid::CreateNull(); // Stores type wrapped by DynamicSerializableField values

        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);
    };

    class ScriptPropertyAsset
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(AZ::ScriptPropertyAsset, "{4D4B7176-A6E1-4BB9-A7B0-5977EC724CCB}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);

        ScriptPropertyAsset() {}
        ScriptPropertyAsset(const char* name)
            : ScriptProperty(name) {}
        virtual ~ScriptPropertyAsset() = default;

        const void* GetDataAddress() const override { return &m_value; }
        const AZ::Uuid& GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyAsset* Clone(const char* name = nullptr) const override;

        bool Write(AZ::ScriptContext& context) override;

        AZ::Data::Asset<AZ::Data::AssetData> m_value;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };
}


