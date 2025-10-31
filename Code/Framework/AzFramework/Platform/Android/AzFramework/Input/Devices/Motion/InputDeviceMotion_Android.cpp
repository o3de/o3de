/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>

#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/Utils.h>

#include <AzCore/Debug/Trace.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for android motion input devices
    class InputDeviceMotionAndroid : public InputDeviceMotion::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMotionAndroid, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMotionAndroid(InputDeviceMotion& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMotionAndroid() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMotion::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMotion::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMotion::Implementation::RefreshMotionSensors
        void RefreshMotionSensors(const InputDeviceRequests::InputChannelIdSet& enabledChannelIds) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the sensor state that is expected by the Java motion manager's RefreshMotionSensors
        //! \param[in] wasRequired Was the sensor required when RefreshMotionSensors was last called?
        //! \param[in] isRequired Is the sensor now required?
        //! \return 1 if the sensor should be enabled, -1 if it should be disabled, 0 for no change
        int GetUpdatedSensorState(bool wasRequired, bool isRequired) const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Check whether a particular sensor is required based on the values of m_enabledChannelIds
        //! \return True if the sensor should be enabled, false otherwise
        bool IsRequiredAccelerationRaw() const;
        bool IsRequiredAccelerationUser() const;
        bool IsRequiredAccelerationGravity() const;
        bool IsRequiredRotationRateRaw() const;
        bool IsRequiredRotationRateUnbiased() const;
        bool IsRequiredMagneticFieldRaw() const;
        bool IsRequiredMagneticFieldUnbiased() const;
        bool IsRequiredOrientationCurrent() const;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Indicies into the motion sensor data returned from the Java code as a packed float array.
        //!
        //! While we would (ideally) like to encapsulate and return all this data using a Java class,
        //! we would then need to 'reach back' through the JNI for each field to access the raw data.
        //! Packing all the data into a float array is more efficient, but at the expense of needing
        //! to access data using pre-defined array indices, instead of explicitly named class fields.
        //!
        //! But while explicitly named class fields provide a modicum of safety, lots of boilerplate
        //! code is needed, and while support for additional sensor data may be added in the future,
        //! the existing set is unlikely to change, so combined with the above mentioned performance
        //! consideration this approach to obtaining raw sensor data seems preferable on most fronts.
        enum PackedSensorDataIndex
        {
            Index_AccelerationRawUpdated        = 0,
            Index_AccelerationRawX              = 1,
            Index_AccelerationRawY              = 2,
            Index_AccelerationRawZ              = 3,
            Index_AccelerationUserUpdated       = 4,
            Index_AccelerationUserX             = 5,
            Index_AccelerationUserY             = 6,
            Index_AccelerationUserZ             = 7,
            Index_AccelerationGravityUpdated    = 8,
            Index_AccelerationGravityX          = 9,
            Index_AccelerationGravityY          = 10,
            Index_AccelerationGravityZ          = 11,
            Index_RotationRateRawUpdated        = 12,
            Index_RotationRateRawX              = 13,
            Index_RotationRateRawY              = 14,
            Index_RotationRateRawZ              = 15,
            Index_RotationRateUnbiasedUpdated   = 16,
            Index_RotationRateUnbiasedX         = 17,
            Index_RotationRateUnbiasedY         = 18,
            Index_RotationRateUnbiasedZ         = 19,
            Index_MagneticFieldRawUpdated       = 20,
            Index_MagneticFieldRawX             = 21,
            Index_MagneticFieldRawY             = 22,
            Index_MagneticFieldRawZ             = 23,
            Index_MagneticFieldUnbiasedUpdated  = 24,
            Index_MagneticFieldUnbiasedX        = 25,
            Index_MagneticFieldUnbiasedY        = 26,
            Index_MagneticFieldUnbiasedZ        = 27,
            Index_OrientationUpdated            = 28,
            Index_OrientationX                  = 29,
            Index_OrientationY                  = 30,
            Index_OrientationZ                  = 31,
            Index_OrientationW                  = 32,
            Index_OrientationAdjustmentRadiansZ = 33,

            // Length of the packed sensor data array
            PackedSensorDataLength
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The set of currently enabled input channel ids
        InputDeviceRequests::InputChannelIdSet m_enabledChannelIds;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The packed sensor data array. This is a member variable to avoid creating it every frame.
        float* m_packedSensorDataArray;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! JNI object reference to the motion sensor manager
        AZStd::unique_ptr<AZ::Android::JNI::Object> m_motionSensorManager;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Java field / function name
        static constexpr const char* s_javaFuntionNameGetMotionSensorManager = "GetMotionSensorManager";
        static constexpr const char* s_javaFuntionNameRefreshMotionSensors = "RefreshMotionSensors";
        static constexpr const char* s_javaFuntionNameRequestLatestMotionSensorData = "RequestLatestMotionSensorData";
        static constexpr const char* s_javaFieldNameMotionSensorDataPackedLength = "MOTION_SENSOR_DATA_PACKED_LENGTH";
        ///@}
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotionAndroid::InputDeviceMotionAndroid(InputDeviceMotion& inputDevice)
        : InputDeviceMotion::Implementation(inputDevice)
        , m_enabledChannelIds()
        , m_packedSensorDataArray(nullptr)
        , m_motionSensorManager()
    {
        // Create a JNI object reference to the motion sensor manager
        m_motionSensorManager.reset(aznew AZ::Android::JNI::Object("com/amazon/lumberyard/input/MotionSensorManager"));

        // Regsiter the Java methods that are required to enable and query for motion sensor data
        m_motionSensorManager->RegisterMethod(s_javaFuntionNameRefreshMotionSensors, "(FIIIIIIII)V");
        m_motionSensorManager->RegisterMethod(s_javaFuntionNameRequestLatestMotionSensorData, "()[F");
        m_motionSensorManager->RegisterStaticField(s_javaFieldNameMotionSensorDataPackedLength, "I");

        // Create the packed sensor data array
        const int packedSensorDataLength = m_motionSensorManager->GetStaticIntField(s_javaFieldNameMotionSensorDataPackedLength);
        AZ_Assert(packedSensorDataLength == PackedSensorDataLength,
            "InputDeviceMotionAndroid::PackedSensorDataLength != MotionSensorManager::MOTION_SENSOR_DATA_PACKED_LENGTH");
        m_packedSensorDataArray = new float[packedSensorDataLength];

        // create the java instance
        [[maybe_unused]] bool ret = m_motionSensorManager->CreateInstance("(Landroid/app/Activity;)V", AZ::Android::Utils::GetActivityRef());
        AZ_Assert(ret, "Failed to create the MotionSensorManager Java instance.");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotionAndroid::~InputDeviceMotionAndroid()
    {
        // Destroy the packed sensor data array
        delete[] m_packedSensorDataArray;

        // Destroy the JNI object reference to the motion sensor manager
        m_motionSensorManager->DestroyInstance();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsConnected() const
    {
        // Motion input is always available on android
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionAndroid::TickInputDevice()
    {
        if (m_enabledChannelIds.empty())
        {
            // Early out to avoid expensive JNI calls.
            return;
        }

        if (JNIEnv* jniEnv = AZ::Android::JNI::GetEnv())
        {
            // Get the latest sensor data through JNI
            jfloatArray latestSensorData = m_motionSensorManager->InvokeObjectMethod<jfloatArray>(s_javaFuntionNameRequestLatestMotionSensorData);
            AZ_Assert(jniEnv->GetArrayLength(latestSensorData) == PackedSensorDataLength,
                "InputDeviceMotionAndroid::PackedSensorDataLength != MotionSensorManager::MOTION_SENSOR_DATA_PACKED_LENGTH");
            jniEnv->GetFloatArrayRegion(latestSensorData, 0, PackedSensorDataLength, m_packedSensorDataArray);
            AZ_Assert(!jniEnv->ExceptionCheck(), "JNI exception thrown while getting the latest sensor data");
            jniEnv->DeleteGlobalRef(latestSensorData);
        }
        else
        {
            // The JNI environment is not valid
            return;
        }

        // While we would (ideally) like to encapsulate and return all this data using a Java class,
        // we would then need to 'reach back' through the JNI for each field to access the raw data.
        // Packing all the data into a float array is more efficient, but at the expense of needing
        // to access data using pre-defined array indices, instead of explicitly named class fields.
        //
        // But while explicitly named class fields provide a modicum of safety, lots of boilerplate
        // code is needed, and while support for additional sensor data may be added in the future,
        // the existing set is unlikely to change, so combined with the above mentioned performance
        // consideration this approach to obtaining raw sensor data seems preferable on most fronts.

        if (m_packedSensorDataArray[Index_AccelerationRawUpdated])
        {
            // Process raw acceleration
            const AZ::Vector3 sensorData(m_packedSensorDataArray[Index_AccelerationRawX],
                                         m_packedSensorDataArray[Index_AccelerationRawY],
                                         m_packedSensorDataArray[Index_AccelerationRawZ]);
            ProcessAccelerationData(InputDeviceMotion::Acceleration::Raw, sensorData);
        }

        if (m_packedSensorDataArray[Index_AccelerationUserUpdated])
        {
            // Process user acceleration
            const AZ::Vector3 sensorData(m_packedSensorDataArray[Index_AccelerationUserX],
                                         m_packedSensorDataArray[Index_AccelerationUserY],
                                         m_packedSensorDataArray[Index_AccelerationUserZ]);
            ProcessAccelerationData(InputDeviceMotion::Acceleration::User, sensorData);
        }

        if (m_packedSensorDataArray[Index_AccelerationGravityUpdated])
        {
            // Process gravity acceleration
            const AZ::Vector3 sensorData(m_packedSensorDataArray[Index_AccelerationGravityX],
                                         m_packedSensorDataArray[Index_AccelerationGravityY],
                                         m_packedSensorDataArray[Index_AccelerationGravityZ]);
            ProcessAccelerationData(InputDeviceMotion::Acceleration::Gravity, sensorData);
        }

        if (m_packedSensorDataArray[Index_RotationRateRawUpdated])
        {
            // Process raw rotation rate
            const AZ::Vector3 sensorData(m_packedSensorDataArray[Index_RotationRateRawX],
                                         m_packedSensorDataArray[Index_RotationRateRawY],
                                         m_packedSensorDataArray[Index_RotationRateRawZ]);
            ProcessRotationRateData(InputDeviceMotion::RotationRate::Raw, sensorData);
        }

        if (m_packedSensorDataArray[Index_RotationRateUnbiasedUpdated])
        {
            // Process unbiased rotation rate
            const AZ::Vector3 sensorData(m_packedSensorDataArray[Index_RotationRateUnbiasedX],
                                         m_packedSensorDataArray[Index_RotationRateUnbiasedY],
                                         m_packedSensorDataArray[Index_RotationRateUnbiasedZ]);
            ProcessRotationRateData(InputDeviceMotion::RotationRate::Unbiased, sensorData);
        }

        if (m_packedSensorDataArray[Index_MagneticFieldRawUpdated])
        {
            // Process raw magnetic field
            const AZ::Vector3 sensorData(m_packedSensorDataArray[Index_MagneticFieldRawX],
                                         m_packedSensorDataArray[Index_MagneticFieldRawY],
                                         m_packedSensorDataArray[Index_MagneticFieldRawZ]);
            ProcessMagneticFieldData(InputDeviceMotion::MagneticField::Raw, sensorData);
        }

        if (m_packedSensorDataArray[Index_MagneticFieldUnbiasedUpdated])
        {
            // Process unbiased magnetic field
            const AZ::Vector3 sensorData(m_packedSensorDataArray[Index_MagneticFieldUnbiasedX],
                                         m_packedSensorDataArray[Index_MagneticFieldUnbiasedY],
                                         m_packedSensorDataArray[Index_MagneticFieldUnbiasedZ]);
            ProcessMagneticFieldData(InputDeviceMotion::MagneticField::Unbiased, sensorData);
        }

        if (m_packedSensorDataArray[Index_AccelerationGravityUpdated] &&
            m_packedSensorDataArray[Index_MagneticFieldUnbiasedUpdated])
        {
            // Calculate and process magnetic north
            const AZ::Vector3 gravity(m_packedSensorDataArray[Index_AccelerationGravityX],
                                      m_packedSensorDataArray[Index_AccelerationGravityY],
                                      m_packedSensorDataArray[Index_AccelerationGravityZ]);
            const AZ::Vector3 magneticField(m_packedSensorDataArray[Index_MagneticFieldUnbiasedX],
                                            m_packedSensorDataArray[Index_MagneticFieldUnbiasedY],
                                            m_packedSensorDataArray[Index_MagneticFieldUnbiasedZ]);
            const AZ::Vector3 gravityNormalized = gravity.GetNormalized();
            const AZ::Vector3 magneticFieldNormalized = magneticField.GetNormalized();
            const AZ::Vector3 magneticEastNormalized = gravityNormalized.Cross(magneticFieldNormalized).GetNormalized();
            const AZ::Vector3 magneticNorthNormalized = magneticEastNormalized.Cross(gravityNormalized).GetNormalized();
            ProcessMagneticFieldData(InputDeviceMotion::MagneticField::North, magneticNorthNormalized);
        }

        if (m_packedSensorDataArray[Index_OrientationUpdated])
        {
            // Process current orientation
            const AZ::Vector3 sensorDataXYZ(m_packedSensorDataArray[Index_OrientationX],
                                            m_packedSensorDataArray[Index_OrientationY],
                                            m_packedSensorDataArray[Index_OrientationZ]);
            const float sensorDataImaginary(m_packedSensorDataArray[Index_OrientationW]);
            AZ::Quaternion sensorData = AZ::Quaternion::CreateFromVector3AndValue(sensorDataXYZ,
                                                                                  sensorDataImaginary);

            // Android doesn't provide us with any quaternion math,
            // so we adjust alignment here instead of the Java code.
            const float rotationAdjustmentZ(m_packedSensorDataArray[Index_OrientationAdjustmentRadiansZ]);
            sensorData *= AZ::Quaternion::CreateRotationZ(rotationAdjustmentZ);

            ProcessOrientationData(InputDeviceMotion::Orientation::Current, sensorData);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionAndroid::RefreshMotionSensors(
        const InputDeviceRequests::InputChannelIdSet& enabledChannelIds)
    {
        if (m_enabledChannelIds == enabledChannelIds)
        {
            // Early out to avoid expensive JNI calls.
            return;
        }

        const bool wasRequiredAccelerationRaw = IsRequiredAccelerationRaw();
        const bool wasRequiredAccelerationUser = IsRequiredAccelerationUser();
        const bool wasRequiredAccelerationGravity = IsRequiredAccelerationGravity();
        const bool wasRequiredRotationRateRaw = IsRequiredRotationRateRaw();
        const bool wasRequiredRotationRateUnbiased = IsRequiredRotationRateUnbiased();
        const bool wasRequiredMagneticFieldRaw = IsRequiredMagneticFieldRaw();
        const bool wasRequiredMagneticFieldUnbiased = IsRequiredMagneticFieldUnbiased();
        const bool wasRequiredOrientationCurrent = IsRequiredOrientationCurrent();

        m_enabledChannelIds = enabledChannelIds;

        const bool isRequiredAccelerationRaw = IsRequiredAccelerationRaw();
        const bool isRequiredAccelerationUser = IsRequiredAccelerationUser();
        const bool isRequiredAccelerationGravity = IsRequiredAccelerationGravity();
        const bool isRequiredRotationRateRaw = IsRequiredRotationRateRaw();
        const bool isRequiredRotationRateUnbiased = IsRequiredRotationRateUnbiased();
        const bool isRequiredMagneticFieldRaw = IsRequiredMagneticFieldRaw();
        const bool isRequiredMagneticFieldUnbiased = IsRequiredMagneticFieldUnbiased();
        const bool isRequiredOrientationCurrent = IsRequiredOrientationCurrent();

        static const float s_desiredMotionSensorUpdateIntervalSeconds = 1.0f / 30.0f;
        m_motionSensorManager->InvokeVoidMethod(
            s_javaFuntionNameRefreshMotionSensors,
            s_desiredMotionSensorUpdateIntervalSeconds,
            GetUpdatedSensorState(wasRequiredAccelerationRaw, isRequiredAccelerationRaw),
            GetUpdatedSensorState(wasRequiredAccelerationUser, isRequiredAccelerationUser),
            GetUpdatedSensorState(wasRequiredAccelerationGravity, isRequiredAccelerationGravity),
            GetUpdatedSensorState(wasRequiredRotationRateRaw, isRequiredRotationRateRaw),
            GetUpdatedSensorState(wasRequiredRotationRateUnbiased, isRequiredRotationRateUnbiased),
            GetUpdatedSensorState(wasRequiredMagneticFieldRaw, isRequiredMagneticFieldRaw),
            GetUpdatedSensorState(wasRequiredMagneticFieldUnbiased, isRequiredMagneticFieldUnbiased),
            GetUpdatedSensorState(wasRequiredOrientationCurrent, isRequiredOrientationCurrent));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    int InputDeviceMotionAndroid::GetUpdatedSensorState(bool wasRequired, bool isRequired) const
    {
        if (wasRequired == isRequired)
        {
            return 0; // Unchanged
        }
        else if (isRequired)
        {
            return 1; // Enable
        }
        else
        {
            return -1; // Disable
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredAccelerationRaw() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::Acceleration::Raw) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredAccelerationUser() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::Acceleration::User) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredAccelerationGravity() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::Acceleration::Gravity) != m_enabledChannelIds.end() ||
               m_enabledChannelIds.find(InputDeviceMotion::MagneticField::North) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredRotationRateRaw() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::RotationRate::Raw) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredRotationRateUnbiased() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::RotationRate::Unbiased) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredMagneticFieldRaw() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::MagneticField::Raw) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredMagneticFieldUnbiased() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::MagneticField::Unbiased) != m_enabledChannelIds.end() ||
               m_enabledChannelIds.find(InputDeviceMotion::MagneticField::North) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionAndroid::IsRequiredOrientationCurrent() const
    {
        return m_enabledChannelIds.find(InputDeviceMotion::Orientation::Current) != m_enabledChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class AndroidDeviceMotionImplFactory
        : public InputDeviceMotion::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMotion::Implementation> Create(InputDeviceMotion& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceMotionAndroid>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        m_deviceMotionImplFactory = AZStd::make_unique<AndroidDeviceMotionImplFactory>();
        AZ::Interface<InputDeviceMotion::ImplementationFactory>::Register(m_deviceMotionImplFactory.get());
    }
} // namespace AzFramework
