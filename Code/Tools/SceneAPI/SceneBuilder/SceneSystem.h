#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    class Vector3;

    namespace SDKScene
    {
        class SceneWrapperBase;
    }

    namespace SceneAPI
    {
        class SceneSystem
        {
        public:
            SceneSystem();

            void Set(const SDKScene::SceneWrapperBase* sceneWrapper);
            void SwapVec3ForUpAxis(Vector3& swapVector) const;
            void SwapTransformForUpAxis(DataTypes::MatrixType& inOutTransform) const;
            void ConvertUnit(Vector3& scaleVector) const;
            void ConvertUnit(DataTypes::MatrixType& inOutTransform) const;
            void ConvertBoneUnit(DataTypes::MatrixType& inOutTransform) const;

            //! Get effect unit size in meters of this scene, internally FBX saves it in the following manner
            //! GlobalSettings:  {
            //!     P : "UnitScaleFactor", "double", "Number", "", 2.54
            float GetUnitSizeInMeters() const { return m_unitSizeInMeters; }
            //! Get original unit size in meters of this scene, internally FBX saves it in the following manner
            //! GlobalSettings:  {
            //!     P : "OriginalUnitScaleFactor", "double", "Number", "", 2.54
            float GetOriginalUnitSizeInMeters() const { return m_originalUnitSizeInMeters; }

        protected:
            float m_unitSizeInMeters = 1;
            float m_originalUnitSizeInMeters = 1;

            AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
            AZStd::unique_ptr<DataTypes::MatrixType> m_adjustTransform;
            AZStd::unique_ptr<DataTypes::MatrixType> m_adjustTransformInverse;
            AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
        };
    }
};
