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

#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace RPI
    {
        MaterialFunctorSourceDataRegistration* MaterialFunctorSourceDataRegistration::Get()
        {
            return Interface<MaterialFunctorSourceDataRegistration>::Get();
        }

        void MaterialFunctorSourceDataRegistration::Init()
        {
            Interface<MaterialFunctorSourceDataRegistration>::Register(this);
        }

        void MaterialFunctorSourceDataRegistration::Shutdown()
        {
            m_materialfunctorMap.clear();
            m_inverseMap.clear();
            Interface<MaterialFunctorSourceDataRegistration>::Unregister(this);
        }

        void MaterialFunctorSourceDataRegistration::RegisterMaterialFunctor(const AZStd::string& functorName, const Uuid& typeId)
        {
            auto iter = m_materialfunctorMap.find(functorName);
            if (iter != m_materialfunctorMap.end())
            {
                if (iter->second != typeId)
                {
                    AZ_Error("Material Functor", false, "Material functor name \"%s\" was already registered with a different type Id!", functorName.c_str());
                }
                else
                {
                    AZ_Warning("Material Functor", false, "Material functor name \"%s\" was already registered!", functorName.c_str());
                }
                return;
            }

            m_materialfunctorMap[functorName] = typeId;
            m_inverseMap[typeId] = functorName;
        }

        Uuid MaterialFunctorSourceDataRegistration::FindMaterialFunctorTypeIdByName(const AZStd::string& functorName)
        {
            auto iter = m_materialfunctorMap.find(functorName);
            if (iter == m_materialfunctorMap.end())
            {
                AZ_Warning("Material Functor", false, "Material functor name \"%s\" was NOT registered!", functorName.c_str());
                return 0;
            }
            return iter->second;
        }

        const AZStd::string MaterialFunctorSourceDataRegistration::FindMaterialFunctorNameByTypeId(const Uuid& typeId)
        {
            auto iter = m_inverseMap.find(typeId);
            if (iter == m_inverseMap.end())
            {
                AZ_Warning("Material Functor", false, "Material functor type \"%s\" was NOT registered!", typeId.ToString<AZStd::string>().c_str());
                return AZStd::string();
            }
            return iter->second;
        }
    }
}
