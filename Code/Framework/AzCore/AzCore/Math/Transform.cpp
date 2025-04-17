/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace 
    {
        class TransformSerializer
            : public SerializeContext::IDataSerializer
        {
        public:
            // number of floats in the serialized representation, 4 for rotation, 1 for scale and 3 for translation
            static constexpr int NumFloats = 8;

            // number of floats in version 1, which used 4 for rotation, 3 for scale and 3 for translation
            static constexpr int NumFloatsVersion1 = 10;

            // number of floats in version 0, which stored a 3x4 matrix
            static constexpr int NumFloatsVersion0 = 12;

            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian) override;
            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian) override;
            size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian) override;
            bool Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian) override;
            bool CompareValueData(const void* lhs, const void* rhs) override;
        };
    }
    namespace Internal
    {
        void TransformDefaultConstructor(Transform* thisPtr)
        {
            new (thisPtr) Transform(Transform::CreateIdentity());
        }

        void TransformSetTranslationGeneric(Transform* thisPtr, ScriptDataContext& dc)
        {
            bool translationIsSet = false;

            if (dc.GetNumArguments() == 3 &&
                dc.IsNumber(0) &&
                dc.IsNumber(1) &&
                dc.IsNumber(2))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                thisPtr->SetTranslation(x, y, z);
                translationIsSet = true;
            }
            else if (dc.GetNumArguments() == 1 &&
                dc.IsClass<Vector3>(0))
            {
                Vector3 translation = Vector3::CreateZero();
                dc.ReadArg(0, translation);
                thisPtr->SetTranslation(translation);
                translationIsSet = true;
            }

            if (!translationIsSet)
            {
                AZ_Error("Script", false, "Transform SetTranslation only supports the following signatures: SetTranslation(Vector3), SetTranslation(number,number,number)");
            }
        }

        void TransformMultiplyGeneric(const Transform* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<Transform>(0))
                {
                    Transform tm = Transform::CreateIdentity();
                    dc.ReadArg(0, tm);
                    Transform result = *thisPtr * tm;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = thisPtr->TransformPoint(vector3);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = thisPtr->TransformPoint(vector4);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Obb>(0))
                {
                    Obb obb;
                    dc.ReadArg(0, obb);
                    Obb result = *thisPtr * obb;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "ScriptTransform multiply should have only 1 argument - Matrix4x4, Vector3 or a Vector4!");
                dc.PushResult(Transform::CreateIdentity());
            }
        }

        void TransformGetBasisAndTranslationMultipleReturn(const Transform* thisPtr, ScriptDataContext& dc)
        {
            Vector3 basisX(Vector3::CreateZero()), basisY(Vector3::CreateZero()), basisZ(Vector3::CreateZero()), translation(Vector3::CreateZero());
            thisPtr->GetBasisAndTranslation(&basisX, &basisY, &basisZ, &translation);
            dc.PushResult(basisX);
            dc.PushResult(basisY);
            dc.PushResult(basisZ);
            dc.PushResult(translation);
        }

        AZ::Transform ConstructTransformFromValues
            ( float v00, float v01, float v02, float v03
            , float v10, float v11, float v12, float v13
            , float v20, float v21, float v22, float v23)
        {
            return AZ::Transform::CreateFromMatrix3x4(Matrix3x4::CreateFromRows
                ( Vector4(v00, v01, v02, v03)
                , Vector4(v10, v11, v12, v13)
                , Vector4(v20, v21, v22, v23)));
        }
    } // namespace Internal

    const Transform g_transformIdentity = Transform::CreateIdentity();

    size_t TransformSerializer::Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian)
    {
        const Transform* transform = reinterpret_cast<const Transform*>(classPtr);
        float data[NumFloats];
        transform->GetRotation().StoreToFloat4(data);
        data[4] = transform->GetUniformScale();
        transform->GetTranslation().StoreToFloat3(&data[5]);

        for (int i = 0; i < NumFloats; i++)
        {
            AZ_SERIALIZE_SWAP_ENDIAN(data[i], isDataBigEndian);
        }

        return static_cast<size_t>(stream.Write(sizeof(data), reinterpret_cast<void*>(&data)));
    }

    size_t TransformSerializer::DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian)
    {
        if (in.GetLength() < NumFloats * sizeof(float))
        {
            return 0;
        }

        float data[NumFloats];
        in.Read(NumFloats * sizeof(float), reinterpret_cast<void*>(&data));
        char textBuffer[256];
        FloatArrayTextSerializer::DataToText(data, NumFloats, textBuffer, sizeof(textBuffer), isDataBigEndian);
        AZStd::string outText = textBuffer;

        return static_cast<size_t>(out.Write(outText.size(), outText.data()));
    }

    size_t TransformSerializer::TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for strtod to be invariant

        const size_t dataBufferSize = AZStd::max(AZStd::max(NumFloatsVersion1, NumFloatsVersion0), NumFloats);
        const size_t numElements = textVersion < 1 ? NumFloatsVersion0 : (textVersion == 1 ? NumFloatsVersion1 : NumFloats);

        size_t nextNumberIndex = 0;
        AZStd::array<float, dataBufferSize> data;
        while (text != nullptr && nextNumberIndex < numElements)
        {
            char* end;
            data[nextNumberIndex] = static_cast<float>(strtod(text, &end));
            AZ_SERIALIZE_SWAP_ENDIAN(data[nextNumberIndex], isDataBigEndian);
            text = end;
            ++nextNumberIndex;
        }

        size_t numBytesRead = nextNumberIndex * sizeof(float);
        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        return static_cast<size_t>(stream.Write(numBytesRead, data.data()));
    }

    bool TransformSerializer::Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian)
    {
        // handle loading of the old data format, 12 floats in a 3x4 matrix
        if (version < 1)
        {
            float data[NumFloatsVersion0];
            if (stream.GetLength() < sizeof(data))
            {
                return false;
            }

            stream.Read(sizeof(data), reinterpret_cast<void*>(data));

            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(data); ++i)
            {
                AZ_SERIALIZE_SWAP_ENDIAN(data[i], isDataBigEndian);
            }

            Matrix3x4 matrix3x4 = Matrix3x4::CreateFromColumnMajorFloat12(data);
            *reinterpret_cast<Transform*>(classPtr) = Transform::CreateFromMatrix3x4(matrix3x4);
            return true;
        }

        // version 1 had a quaternion rotation, vector3 scale and vector3 translation
        else if (version == 1)
        {
            float data[NumFloatsVersion1];
            if (stream.GetLength() < sizeof(data))
            {
                return false;
            }

            stream.Read(sizeof(data), reinterpret_cast<void*>(data));

            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(data); ++i)
            {
                AZ_SERIALIZE_SWAP_ENDIAN(data[i], isDataBigEndian);
            }

            Quaternion rotation = Quaternion::CreateFromFloat4(data);
            Vector3 vectorScale = Vector3::CreateFromFloat3(&data[4]);
            Vector3 translation = Vector3::CreateFromFloat3(&data[7]);

            float uniformScale = vectorScale.GetMaxElement();

            *reinterpret_cast<Transform*>(classPtr) =
                Transform::CreateFromQuaternionAndTranslation(rotation, translation) * Transform::CreateUniformScale(uniformScale);
            return true;
        }

        // otherwise load as a quaternion rotation, float scale and vector3 translation
        float data[NumFloats];
        if (stream.GetLength() < sizeof(data))
        {
            return false;
        }

        stream.Read(sizeof(data), reinterpret_cast<void*>(data));

        for (unsigned int i = 0; i < AZ_ARRAY_SIZE(data); ++i)
        {
            AZ_SERIALIZE_SWAP_ENDIAN(data[i], isDataBigEndian);
        }

        Quaternion rotation = Quaternion::CreateFromFloat4(data);
        float scale = data[4];
        Vector3 translation = Vector3::CreateFromFloat3(&data[5]);

        *reinterpret_cast<Transform*>(classPtr) =
            Transform::CreateFromQuaternionAndTranslation(rotation, translation) * Transform::CreateUniformScale(scale);
        return true;
    }

    bool TransformSerializer::CompareValueData(const void* lhs, const void* rhs)
    {
        const Transform* transformLhs = reinterpret_cast<const Transform*>(lhs);
        const Transform* transformRhs = reinterpret_cast<const Transform*>(rhs);
        return transformLhs->IsClose(*transformRhs, GetTransformEpsilon());
    }

    void Transform::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Transform>()
                ->Version(2)
                ->Serializer<TransformSerializer>();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Transform>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::TransformDefaultConstructor)->
                Constructor<const Vector3&, const Quaternion&, float>()->
                Method("GetBasis", &Transform::GetBasis)->
                Method("GetBasisX", &Transform::GetBasisX)->
                Method("GetBasisY", &Transform::GetBasisY)->
                Method("GetBasisZ", &Transform::GetBasisZ)->
                Property<const Vector3&(Transform::*)() const, void (Transform::*)(const Vector3&)>("translation", &Transform::GetTranslation, &Transform::SetTranslation)->
                Property<const Quaternion&(Transform::*)() const, void (Transform::*)(const Quaternion&)>("rotation", &Transform::GetRotation, &Transform::SetRotation)->
                Method("ToString", &TransformToString)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::ToString)->
                Method<Vector3(Transform::*)(const Vector3&) const>("Multiply", &Transform::TransformPoint)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::TransformMultiplyGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Mul)->
                Method<Vector4(Transform::*)(const Vector4&) const>("MultiplyVector4", &Transform::TransformPoint)->
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Method<Transform(Transform::*)(const Transform&) const>("MultiplyTransform", &Transform::operator*)->
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("Equal", &Transform::operator==)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Equal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("Clone", [](const Transform& rhs) -> Transform { return rhs; })->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("GetTranslation", &Transform::GetTranslation)->
                Method("GetBasisAndTranslation", &Transform::GetBasisAndTranslation)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::TransformGetBasisAndTranslationMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("TransformVector", &Transform::TransformVector)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method<void (Transform::*)(const Vector3&)>("SetTranslation", &Transform::SetTranslation)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::TransformSetTranslationGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("GetRotation", &Transform::GetRotation)->
                Method<void (Transform::*)(const Quaternion&)>("SetRotation", &Transform::SetRotation)->
                Method("GetUniformScale", &Transform::GetUniformScale)->
                Method("SetUniformScale", &Transform::SetUniformScale)->
                Method("ExtractUniformScale", &Transform::ExtractUniformScale)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("MultiplyByUniformScale", &Transform::MultiplyByUniformScale)->
                Method("GetInverse", &Transform::GetInverse)->
                Method("Invert", &Transform::Invert)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("IsOrthogonal", &Transform::IsOrthogonal, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("GetOrthogonalized", &Transform::GetOrthogonalized)->
                Method("Orthogonalize", &Transform::Orthogonalize)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Method("IsClose", &Transform::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("IsFinite", &Transform::IsFinite)->
                Method("CreateIdentity", &Transform::CreateIdentity)->
                Method("CreateRotationX", &Transform::CreateRotationX)->
                Method("CreateRotationY", &Transform::CreateRotationY)->
                Method("CreateRotationZ", &Transform::CreateRotationZ)->
                Method("CreateFromQuaternion", &Transform::CreateFromQuaternion)->
                Method("CreateFromQuaternionAndTranslation", &Transform::CreateFromQuaternionAndTranslation)->
                Method("CreateFromMatrix3x3", &Transform::CreateFromMatrix3x3)->
                Method("CreateFromMatrix3x3AndTranslation", &Transform::CreateFromMatrix3x3AndTranslation)->
                Method("CreateUniformScale", &Transform::CreateUniformScale)->
                Method("CreateTranslation", &Transform::CreateTranslation)->
                Method("CreateLookAt", &Transform::CreateLookAt)->
                Method("GetEulerDegrees", &Transform::GetEulerDegrees)->
                Method("ConstructFromValuesNumeric", &Internal::ConstructTransformFromValues);
        }
    }

    Transform Transform::CreateFromMatrix3x3(const Matrix3x3& value)
    {
        Transform result;
        Matrix3x3 tmp = value;
        result.m_scale = tmp.ExtractScale().GetMaxElement();
        result.m_rotation = Quaternion::CreateFromMatrix3x3(tmp);
        result.m_translation = Vector3::CreateZero();
        return result;
    }

    Transform Transform::CreateFromMatrix3x3AndTranslation(const class Matrix3x3& value, const Vector3& p)
    {
        Transform result;
        Matrix3x3 tmp = value;
        result.m_scale = tmp.ExtractScale().GetMaxElement();
        result.m_rotation = Quaternion::CreateFromMatrix3x3(tmp);
        result.m_translation = p;
        return result;
    }

    Transform Transform::CreateFromMatrix3x4(const Matrix3x4& value)
    {
        Transform result;
        Matrix3x4 tmp = value;
        result.m_scale = tmp.ExtractScale().GetMaxElement();
        result.m_rotation = Quaternion::CreateFromMatrix3x4(tmp);
        result.m_translation = value.GetTranslation();
        return result;
    }

    Transform Transform::CreateLookAt(const Vector3& from, const Vector3& to, Transform::Axis forwardAxis)
    {
        Transform result = CreateIdentity();

        Vector3 targetForward = to - from;

        if (targetForward.IsZero())
        {
            AZ_Error("Transform", false, "Can't create look-at matrix when 'to' and 'from' positions are equal.");
            return result;
        }

        targetForward.Normalize();

        // Open 3D Engine is Z-up and is right-handed.
        Vector3 up = Vector3::CreateAxisZ();

        // We have a degenerate case if target forward is parallel to the up axis,
        // in which case we'll have to pick a new up vector
        const float absDot = Abs(targetForward.Dot(up));
        if (absDot > 1.0f - 0.001f)
        {
            up = targetForward.CrossYAxis();
        }

        Vector3 right = targetForward.Cross(up);
        right.Normalize();
        up = right.Cross(targetForward);
        up.Normalize();

        // Passing in forwardAxis allows you to force a particular local-space axis to look
        // at the target point.  In Open 3D Engine, the default is forward is along Y+.
        switch (forwardAxis)
        {
        case Axis::XPositive:
            result.m_rotation = Quaternion::CreateFromBasis(targetForward, -right, up);
            break;
        case Axis::XNegative:
            result.m_rotation = Quaternion::CreateFromBasis(-targetForward, right, up);
            break;
        case Axis::YPositive:
            result.m_rotation = Quaternion::CreateFromBasis(right, targetForward, up);
            break;
        case Axis::YNegative:
            result.m_rotation = Quaternion::CreateFromBasis(-right, -targetForward, up);
            break;
        case Axis::ZPositive:
            result.m_rotation = Quaternion::CreateFromBasis(right, -up, targetForward);
            break;
        case Axis::ZNegative:
            result.m_rotation = Quaternion::CreateFromBasis(right, up, -targetForward);
            break;
        default:
            result.m_rotation = Quaternion::CreateFromBasis(right, targetForward, up);
            break;
        }
        result.SetTranslation(from);

        return result;
    }
}
