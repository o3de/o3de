/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/AZStdAnyDataContainer.inl>

namespace ScriptEventData
{

    struct VoidType
    {
        AZ_TYPE_INFO(VoidType, "{BFF11497-FBD1-460A-B21F-D4519B9123ED}");
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<VoidType>();
            }
        }
    };

    class VersionedProperty;

    //! A VersionedProperty holds a default or starting value and a list of versions.
    //! The list of versions is immutable until the moment the property is flattened.
    //! Flattening a property discards all versioning information but the latest and can
    //! be used when it is desired to reduce the data size footprint.
    //! Keeping the versioning data around is incredibly handly for backwards compatibility.
    class VersionedProperty
    {
    public:
        AZ_RTTI(VersionedProperty, "{828CA9C0-32F1-40B3-8018-EE7C3C38192A}");
        AZ_CLASS_ALLOCATOR(VersionedProperty, AZ::SystemAllocator);

        VersionedProperty()
            : m_id(AZ::Uuid::CreateRandom())
            , m_version(0)
            , m_backup(nullptr)
        {
            m_label = "MISSING_LABEL";
        }

        VersionedProperty(AZStd::string label)
            : m_id(AZ::Uuid::CreateRandom())
            , m_version(0)
            , m_label(label)
            , m_backup(nullptr)
        {
        }

        VersionedProperty(const VersionedProperty& rhs)
        {
            m_id = rhs.m_id;
            m_version = rhs.m_version;
            m_label = rhs.m_label;

            m_data = rhs.m_data;
            m_versions = rhs.m_versions;
        }

        VersionedProperty(VersionedProperty&& rhs)
        {
            m_id = AZStd::move(rhs.m_id);
            m_version = AZStd::move(rhs.m_version);
            m_label = AZStd::move(rhs.m_label);

            m_data = AZStd::move(rhs.m_data);
            m_versions.swap(rhs.m_versions);
        }

        VersionedProperty& operator=(const VersionedProperty& rhs)
        {
            if (this != &rhs)
            {
                m_id = rhs.m_id;
                m_version = rhs.m_version;
                m_label = rhs.m_label;

                m_data = rhs.m_data;
                m_versions.assign(rhs.m_versions.begin(), rhs.m_versions.end());
            }

            return *this;
        }

        VersionedProperty(AZ::ScriptDataContext&);
        virtual ~VersionedProperty()
        {
            m_data.clear();
            delete m_backup;
            m_versions.clear();
        }

        AZ::Uuid GetType() const
        {
            return m_data.type();
        }

        static VersionedProperty MakeVoid()
        {
            VersionedProperty property = VersionedProperty("Void");
            property.Set<const VoidType>(VoidType {});
            return property;
        }

        template <typename T>
        static VersionedProperty Make(T t, const char* label)
        {
            VersionedProperty p(label);
            p.Set(t);
            return p;
        }

        void SetLabel(const char* label)
        {
            m_label = label;
        }

        // Return by value
        AZStd::string GetLabel() const { return m_label; }

        bool operator==(const VersionedProperty& rhs) const
        {
            return AZ::Helpers::CompareAnyValue(m_data, rhs.m_data);
        }

        AZStd::string ToString() const
        {
            return m_id.ToString<AZStd::string>();
        }

        bool operator!=(const VersionedProperty& rhs) const
        {
            return !operator==(rhs);
        }

        static void Reflect(AZ::ReflectContext* context);

        void IncreaseVersion() { m_version++; }

        void Get(AZ::ScriptDataContext& dc);
        void Set(AZ::ScriptDataContext& dc);

        bool IsEmpty() const 
        {
            return m_data.empty();
        }

        struct VersionSort
        {
            inline bool operator() (const VersionedProperty& a, const VersionedProperty& b)
            {
                return (a.m_version > b.m_version);
            }
        };

        //! Data can only be set into a property through this function.
        template <typename T>
        void Set(T&& data)
        {
            m_data = AZStd::any(AZStd::forward<T>(data));
        }

        //! Returns the latest version of the property
        template <typename T>
        const T* Get() const
        {
            if (m_data.empty())
            {
                return nullptr;
            }

            return AZStd::any_cast<T>(&m_data);
        }

        template <typename T>
        bool Get(T& out) const
        {
            if (!m_data.empty())
            {
                out = AZStd::any_cast<T>(m_data);
                return true;
            }
            return false;
        }

        template <typename T>
        T* Get()
        {
            if (m_data.empty())
            {
                return nullptr;
            }
            return AZStd::any_cast<T>(&m_data);
        }

        void Set(const char* str)
        {
            AZStd::string text = str;
            Set<const AZStd::string>(AZStd::move(text));
        }

        //! Creates a new version of the desired property.
        VersionedProperty& NewVersion()
        {
            if (m_backup)
            {
                m_backup->m_versions.clear(); // Do not store the backup version history, we only need the property, otherwise this leads to exponential growth
                VersionedProperty copy = *m_backup;
                m_versions.push_back(AZStd::move(copy));
                delete m_backup;
                m_backup = nullptr;

                ++m_version;
            }

            return *this;
        }

        void OnPropertyChange()
        {
            if (!m_backup)
            {
                m_backup = aznew VersionedProperty(*this);
            }
        }

        //! Applies the latest version as the default and clears the versioned information.
        //! Warning: This operation is intentionally destructive, if the asset is saved after flattening
        //! The versioning information will be lost, however, the asset size will be reduced.
        void Flatten()
        {
            ApplyLatestVersions();
            m_versions.clear();
        }

        //! Applies the latest version as the default, it can be used to make it easy to get access
        //! to the latest version.
        void ApplyLatestVersions()
        {
            for (const auto& property : m_versions)
            {
                if (property.m_version > m_version)
                {
                    m_version = property.m_version;
                    m_data = property.m_data;
                }
            }
        }

        AZ::Uuid GetId() const { return m_id; }
        AZ::u32 GetVersion() const { return m_version; }

        const AZStd::vector<VersionedProperty>& GetVersions() const { return m_versions; }

        const AZStd::any& GetRaw() const { return m_data; }

        template <typename T>
        void SetDefaultFromType()
        {
            m_data = AZStd::make_any<T>();
        }

        void PreSave();

    private:

        AZ::Uuid m_id;
        AZ::u32 m_version;
        AZStd::any m_data;
        AZStd::string m_label;

        AZStd::vector<VersionedProperty> m_versions;

        VersionedProperty* m_backup = nullptr;
    };

    //! Given a class that may hold any VersionedProperties, iterate over its elements and 
    //! if any elements are VersionedProperty, they will be flattened.
    template <typename T>
    void FlattenVersionedPropertiesInObject(AZ::SerializeContext* serializeContext, T* obj)
    {
        serializeContext->EnumerateObject(obj, 
            [](void *instance, 
               const AZ::SerializeContext::ClassData *classData, 
               [[maybe_unused]] const AZ::SerializeContext::ClassElement *classElement) -> bool
            {
                if (classData->m_typeId == azrtti_typeid<VersionedProperty>())
                {
                    auto property = reinterpret_cast<VersionedProperty*>(instance);
                    if (property != nullptr)
                    {
                        property->Flatten();
                    }
                }
                return true;
            },

            []() -> bool { return true; },
            AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr);
    }
}
