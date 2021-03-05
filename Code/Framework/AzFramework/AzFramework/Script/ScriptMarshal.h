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
#ifndef AZFRAMEWORK_SCRIPT_SCRIPTMARSHAL_H
#define AZFRAMEWORK_SCRIPT_SCRIPTMARSHAL_H

#include <GridMate/Serialize/ContainerMarshal.h>

#include <AzCore/RTTI/BehaviorObjectSignals.h>

namespace AZ
{
    class ScriptProperty;
}

namespace AzFramework
{
    /**
     * Specalized helper marshaler for ScriptProperty class
     */
    class ScriptPropertyMarshaler
    {
    public:
        void Marshal(GridMate::WriteBuffer& wb, AZ::ScriptProperty*const& cont) const;
        bool UnmarshalToPointer(AZ::ScriptProperty*& target, GridMate::ReadBuffer& rb) const;
    };

    class ScriptPropertyThrottler
    {
    public:
        ScriptPropertyThrottler();

        void SignalDirty();
        bool WithinThreshold(AZ::ScriptProperty* newValue) const;
        void UpdateBaseline(AZ::ScriptProperty* baseline);

    private:        
        bool m_isDirty;
    };

    /**
     * Specialized helper marshaler to help with the vector creation/destruction
     */
    class ScriptRPCMarshaler
    {
    public:

        typedef AZStd::vector< AZ::ScriptProperty* > Container;

        ScriptRPCMarshaler()            
        {
        }

        AZ_FORCE_INLINE void Marshal(GridMate::WriteBuffer& wb, const Container& container) const
        {
            AZ_Assert(container.size() < USHRT_MAX, "Container has too many elements for marshaling!");
            AZ::u16 size = static_cast<AZ::u16>(container.size());
            wb.Write(size);
            for (const auto& i : container)
            {
                m_marshaler.Marshal(wb, i);
            }
        }

        AZ_FORCE_INLINE void Unmarshal(Container& container, GridMate::ReadBuffer& rb) const
        {
            container.clear();

            AZ::u16 size;
            rb.Read(size);
            container.reserve(size);

            for (AZ::u16 i = 0; i < size; ++i)
            {
                AZ::ScriptProperty* readProperty = nullptr;
                m_marshaler.UnmarshalToPointer(readProperty, rb);
                container.insert(container.end(), readProperty);
            }
        }

    protected:
        ScriptPropertyMarshaler m_marshaler;
    };
}

#endif