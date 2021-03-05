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
#ifndef INCLUDE_GRIDMATENETSERIALIZEASPECTPROFILES_HEADER
#define INCLUDE_GRIDMATENETSERIALIZEASPECTPROFILES_HEADER

#pragma once

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Replica/DataSet.h>

#include "GridMateNetSerialize.h"

namespace GridMate
{
    namespace NetSerialize
    {
        typedef uint8 AspectProfile;
        static const AspectProfile kUnsetAspectProfile = (AspectProfile)(~0);

        /*!
         * Marshable list of aspect profiles.
         */
        class EntityAspectProfiles
        {
        public:
            EntityAspectProfiles();

            void SetAspectProfile(size_t aspectIndex, AspectProfile profile);

            AspectProfile GetAspectProfile(size_t aspectIndex) const;

            bool operator == (const EntityAspectProfiles& other) const;

            class Marshaler
            {
            public:
                typedef AZStd::function<void(size_t /*aspectIndex*/,
                                             AspectProfile /*oldProfile*/,
                                             AspectProfile /*newProfile*/)> ChangeDelegate;

                void Marshal(GridMate::WriteBuffer& wb, const EntityAspectProfiles& s);
                void Unmarshal(EntityAspectProfiles& s, GridMate::ReadBuffer& rb);

                void SetChangeDelegate(ChangeDelegate changeDelegate);

            private:

                ChangeDelegate m_changeDelegate;

                GridMate::Marshaler<AspectProfile> m_profileMarshaler;
            };

        private:
            AZ::u32 m_profilesMask;
            AspectProfile m_aspectProfiles[ NetSerialize::kNumAspectSlots ] = { kUnsetAspectProfile };
        };

        typedef GridMate::DataSet<EntityAspectProfiles, EntityAspectProfiles::Marshaler>
            SerializedEntityAspectProfiles;
    } // namespace NetSerialize
} // namespace GridMate

#endif // INCLUDE_GRIDMATENETSERIALIZEASPECTPROFILES_HEADER
