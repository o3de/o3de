/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/RTTI.h>

namespace AzToolsFramework
{
    //! Provide unique type alias for AZ::u64 for manipulator, bounds and manager.
    template<typename T>
    class IdType
    {
    public:
        explicit IdType(AZ::u64 id = 0)
            : m_id(id)
        {
        }

        operator AZ::u64() const
        {
            return m_id;
        }

        bool operator==(IdType other) const
        {
            return m_id == other.m_id;
        }

        bool operator!=(IdType other) const
        {
            return m_id != other.m_id;
        }

        IdType& operator++() // pre-increment
        {
            ++m_id;
            return *this;
        }

        IdType operator++(int) // post-increment
        {
            IdType temp = *this;
            ++*this;
            return temp;
        }

    private:
        AZ::u64 m_id;
    };

    namespace Picking
    {
        class BoundRequestShapeBase;

        using RegisteredBoundId = IdType<struct RegisteredBoundType>;
        static const RegisteredBoundId InvalidBoundId = RegisteredBoundId(0);

        //! This class serves as the base class for the actual bound shapes that various DefaultContextBoundManager-derived
        //! classes return from the function CreateShape.
        class BoundShapeInterface
        {
        public:
            AZ_RTTI(BoundShapeInterface, "{C639CB8E-1957-4E4F-B889-3BE1DFBC358D}");

            explicit BoundShapeInterface(const RegisteredBoundId boundId)
                : m_boundId(boundId)
                , m_valid(false)
            {
            }

            virtual ~BoundShapeInterface() = default;

            RegisteredBoundId GetBoundId() const
            {
                return m_boundId;
            }

            //! @param rayOrigin The origin of the ray to test with.
            //! @param rayDir The direction of the ray to test with.
            //! @param[out] rayIntersectionDistance The distance of the intersecting point closest to the ray origin.
            //! @return Boolean indicating whether there is a least one intersecting point between this bound shape and the ray.
            virtual bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const = 0;

            virtual void SetShapeData(const BoundRequestShapeBase& shapeData) = 0;

            void SetValidity(bool valid)
            {
                m_valid = valid;
            }

            bool IsValid() const
            {
                return m_valid;
            }

        private:
            RegisteredBoundId m_boundId;
            bool m_valid;
        };
    } // namespace Picking
} // namespace AzToolsFramework
