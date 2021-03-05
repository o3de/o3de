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
#ifndef AZCORE_DYNAMIC_SERIALIZABLE_FIELD_H
#define AZCORE_DYNAMIC_SERIALIZABLE_FIELD_H

#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    struct Uuid;
    class SerializeContext;

    /*
     * Allows the user to serialize data using a void* and Uuid pair.
     * It is currently used by the script component. We could use this for
     * other purposes but we may have to make it more robust and user-friendly.
     */
    class DynamicSerializableField
    {
    public:
        AZ_TYPE_INFO(DynamicSerializableField, "{D761E0C2-A098-497C-B8EB-EA62F5ED896B}")

        DynamicSerializableField();
        DynamicSerializableField(const DynamicSerializableField& serializableField);
        ~DynamicSerializableField();

        bool IsValid() const;
        void DestroyData(SerializeContext* useContext = nullptr);
        void* CloneData(SerializeContext* useContext = nullptr) const;

        void CopyDataFrom(const DynamicSerializableField& other);        
        bool IsEqualTo(const DynamicSerializableField& other, SerializeContext* useContext = nullptr);

        template<class T>
        void Set(T* object)
        {
            m_data = object;
            m_typeId = AzTypeInfo<T>::Uuid();
        }

        // Keep in mind that this function will not do rtti_casts to T, you will need to
        // an instance of serialize context for that.
        template<class T>
        T* Get() const
        {
            if (m_typeId == AzTypeInfo<T>::Uuid())
            {
                return reinterpret_cast<T*>(m_data);
            }
            return nullptr;
        }
    
        void* m_data;
        Uuid m_typeId;
    };
}   // namespace AZ

#endif
