/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>

#include <CoreMotion/CoreMotion.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for ios motion input devices
    class InputDeviceMotionIos : public InputDeviceMotion::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMotionIos, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMotionIos(InputDeviceMotion& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMotionIos() override;

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
        //! Refresh the active state of the accelerometer based on the channels that are enabled
        //! \param[in] enabledChannelIds Set of motion input channel ids that should be enabled
        void RefreshAccelerometer(const InputDeviceRequests::InputChannelIdSet& enabledChannelIds);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh the active state of the gyroscope based on the channels that are enabled
        //! \param[in] enabledChannelIds Set of motion input channel ids that should be enabled
        void RefreshGyroscope(const InputDeviceRequests::InputChannelIdSet& enabledChannelIds);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh the active state of the magnetometer based on the channels that are enabled
        //! \param[in] enabledChannelIds Set of motion input channel ids that should be enabled
        void RefreshMagnetometer(const InputDeviceRequests::InputChannelIdSet& enabledChannelIds);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh the active state of device motion based on the channels that are enabled
        //! \param[in] enabledChannelIds Set of motion input channel ids that should be enabled
        void RefreshDeviceMotion(const InputDeviceRequests::InputChannelIdSet& enabledChannelIds);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Given a vector relative to a device held upright in portrait orientation as shown below:
        //!
        //!  _________          +y   -z
        //! |         |           |  /
        //! |         |           | /
        //! |  Device |           |/          - Right-handed, y-up coordinate system
        //! |  Screen |   -x______|______+x   - The +y axis points out of the top of the device
        //! |         |          /|           - The +z axis points out of the front of the screen
        //! |         |         / |
        //! |_________|        /  |
        //! |____O____|       +z  -y
        //!
        //! return another vector relative to the specified display orientation, and such that the
        //! +y axis points out the back of the screen and z+ axis points out the top of the device.
        //! This flipping of axes is to match Open 3D Engine's z-up and left-handed coordinate system.
        //!
        //! \param[in] x The x component of the vector to be aligned
        //! \param[in] y The y component of the vector to be aligned
        //! \param[in] z The z component of the vector to be aligned
        //! \param[in] displayOrientation The orientation to make the vector relative to
        //! \return A vector made relative to the specified display orientation
        AZ::Vector3 MakeVectorRelativeToOrientation(float x, float y, float z,
                                                    UIInterfaceOrientation displayOrientation);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Given a quaternion relative to a device held upright in portrait orientation,
        //! return another quaternion relative to the specified display orientation.
        //!
        //! \param[in] x The x component of the quaternion to be aligned
        //! \param[in] y The y component of the quaternion to be aligned
        //! \param[in] z The z component of the quaternion to be aligned
        //! \param[in] displayOrientation The orientation to make the vector relative to
        //! \return A quaternion made relative to the specified display orientation
        AZ::Quaternion MakeQuaternionRelativeToOrientation(float w, float x, float y, float z,
                                                           UIInterfaceOrientation displayOrientation);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Variables
        CMMotionManager* m_motionManager; //!< Reference to the motion manager
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotionIos::InputDeviceMotionIos(InputDeviceMotion& inputDevice)
        : InputDeviceMotion::Implementation(inputDevice)
    {
        m_motionManager = [[CMMotionManager alloc] init];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotionIos::~InputDeviceMotionIos()
    {
        [m_motionManager release];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotionIos::IsConnected() const
    {
        // Motion input is always available on ios
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionIos::TickInputDevice()
    {
        UIInterfaceOrientation uiOrientation = UIInterfaceOrientationUnknown;
#if defined(__IPHONE_13_0) || defined(__TVOS_13_0)
        if(@available(iOS 13.0, tvOS 13.0, *))
        {
            UIWindow* foundWindow = nil;
            NSArray* windows = [[UIApplication sharedApplication] windows];
            for (UIWindow* window in windows)
            {
                if (window.isKeyWindow)
                {
                    foundWindow = window;
                    break;
                }
            }
            UIWindowScene* windowScene = foundWindow ? foundWindow.windowScene : nullptr;
            AZ_Assert(windowScene, "WindowScene is invalid");
            if(windowScene)
            {
                uiOrientation = windowScene.interfaceOrientation;
            }
        }
#else
        uiOrientation = UIApplication.sharedApplication.statusBarOrientation;
#endif
        
        if (CMAccelerometerData* accelerometerData = m_motionManager.accelerometerData)
        {
            // Process raw acceleration
            const AZ::Vector3 accelerationRaw = MakeVectorRelativeToOrientation(accelerometerData.acceleration.x,
                                                                                accelerometerData.acceleration.y,
                                                                                accelerometerData.acceleration.z,
                                                                                uiOrientation);
            ProcessAccelerationData(InputDeviceMotion::Acceleration::Raw, accelerationRaw);
        }

        if (CMGyroData* gyroData = m_motionManager.gyroData)
        {
            // Process raw rotation rate
            const AZ::Vector3 rotationRateRaw = MakeVectorRelativeToOrientation(gyroData.rotationRate.x,
                                                                                gyroData.rotationRate.y,
                                                                                gyroData.rotationRate.z,
                                                                                uiOrientation);
            ProcessRotationRateData(InputDeviceMotion::RotationRate::Raw, rotationRateRaw);
        }

        if (CMMagnetometerData* magnetometerData = m_motionManager.magnetometerData)
        {
            // Process raw magnetic field
            const AZ::Vector3 magneticFieldRaw = MakeVectorRelativeToOrientation(magnetometerData.magneticField.x,
                                                                                 magnetometerData.magneticField.y,
                                                                                 magnetometerData.magneticField.z,
                                                                                 uiOrientation);
            ProcessMagneticFieldData(InputDeviceMotion::MagneticField::Raw, magneticFieldRaw);
        }

        if (CMDeviceMotion* deviceMotion = m_motionManager.deviceMotion)
        {
            // Process user acceleration
            const AZ::Vector3 accelerationUser = MakeVectorRelativeToOrientation(deviceMotion.userAcceleration.x,
                                                                                 deviceMotion.userAcceleration.y,
                                                                                 deviceMotion.userAcceleration.z,
                                                                                 uiOrientation);
            ProcessAccelerationData(InputDeviceMotion::Acceleration::User, accelerationUser);

            // Process gravity acceleration
            const AZ::Vector3 accelerationGravity = MakeVectorRelativeToOrientation(deviceMotion.gravity.x,
                                                                                    deviceMotion.gravity.y,
                                                                                    deviceMotion.gravity.z,
                                                                                    uiOrientation);
            ProcessAccelerationData(InputDeviceMotion::Acceleration::Gravity, accelerationGravity);

            // Process unbiased rotation rate
            const AZ::Vector3 rotationRateUnbiased = MakeVectorRelativeToOrientation(deviceMotion.rotationRate.x,
                                                                                     deviceMotion.rotationRate.y,
                                                                                     deviceMotion.rotationRate.z,
                                                                                     uiOrientation);
            ProcessRotationRateData(InputDeviceMotion::RotationRate::Unbiased, rotationRateUnbiased);

            if (deviceMotion.magneticField.accuracy != CMMagneticFieldCalibrationAccuracyUncalibrated)
            {
                // Process unbiased magnetic field
                const AZ::Vector3 magneticFieldUnbiased = MakeVectorRelativeToOrientation(deviceMotion.magneticField.field.x,
                                                                                          deviceMotion.magneticField.field.y,
                                                                                          deviceMotion.magneticField.field.z,
                                                                                          uiOrientation);
                ProcessMagneticFieldData(InputDeviceMotion::MagneticField::Unbiased, magneticFieldUnbiased);

                // Calculate and process magnetic north
                const AZ::Vector3 gravityNormalized = accelerationGravity.GetNormalized();
                const AZ::Vector3 magneticFieldNormalized = magneticFieldUnbiased.GetNormalized();
                const AZ::Vector3 magneticEastNormalized = gravityNormalized.Cross(magneticFieldNormalized).GetNormalized();
                const AZ::Vector3 magneticNorthNormalized = magneticEastNormalized.Cross(gravityNormalized).GetNormalized();
                ProcessMagneticFieldData(InputDeviceMotion::MagneticField::North, magneticNorthNormalized);
            }

            // Process current orientation
            const AZ::Quaternion orientation = MakeQuaternionRelativeToOrientation(deviceMotion.attitude.quaternion.w,
                                                                                   deviceMotion.attitude.quaternion.x,
                                                                                   deviceMotion.attitude.quaternion.y,
                                                                                   deviceMotion.attitude.quaternion.z,
                                                                                   uiOrientation);
            ProcessOrientationData(InputDeviceMotion::Orientation::Current, orientation);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionIos::RefreshMotionSensors(
        const InputDeviceRequests::InputChannelIdSet& enabledChannelIds)
    {
        RefreshAccelerometer(enabledChannelIds);
        RefreshGyroscope(enabledChannelIds);
        RefreshMagnetometer(enabledChannelIds);
        RefreshDeviceMotion(enabledChannelIds);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionIos::RefreshAccelerometer(
        const InputDeviceRequests::InputChannelIdSet& enabledChannelIds)
    {
        const bool shouldBeActive = enabledChannelIds.find(InputDeviceMotion::Acceleration::Raw) != enabledChannelIds.end();
        if (shouldBeActive == m_motionManager.accelerometerActive)
        {
            return;
        }

        if (!shouldBeActive)
        {
            [m_motionManager stopAccelerometerUpdates];
            return;
        }

        if (!m_motionManager.accelerometerAvailable)
        {
            return;
        }

        [m_motionManager startAccelerometerUpdates];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionIos::RefreshGyroscope(
        const InputDeviceRequests::InputChannelIdSet& enabledChannelIds)
    {
        const bool shouldBeActive = enabledChannelIds.find(InputDeviceMotion::RotationRate::Raw) != enabledChannelIds.end();
        if (shouldBeActive == m_motionManager.gyroActive)
        {
            return;
        }

        if (!shouldBeActive)
        {
            [m_motionManager stopGyroUpdates];
            return;
        }

        if (!m_motionManager.gyroAvailable)
        {
            return;
        }

        [m_motionManager startGyroUpdates];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionIos::RefreshMagnetometer(
        const InputDeviceRequests::InputChannelIdSet& enabledChannelIds)
    {
        const bool shouldBeActive = enabledChannelIds.find(InputDeviceMotion::MagneticField::Raw) != enabledChannelIds.end();
        if (shouldBeActive == m_motionManager.magnetometerActive)
        {
            return;
        }

        if (!shouldBeActive)
        {
            [m_motionManager stopMagnetometerUpdates];
            return;
        }

        if (!m_motionManager.magnetometerAvailable)
        {
            return;
        }

        [m_motionManager startMagnetometerUpdates];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotionIos::RefreshDeviceMotion(
        const InputDeviceRequests::InputChannelIdSet& enabledChannelIds)
    {
        const bool shouldBeActive = enabledChannelIds.find(InputDeviceMotion::Acceleration::User) != enabledChannelIds.end() ||
                                    enabledChannelIds.find(InputDeviceMotion::Acceleration::Gravity) != enabledChannelIds.end() ||
                                    enabledChannelIds.find(InputDeviceMotion::RotationRate::Unbiased) != enabledChannelIds.end() ||
                                    enabledChannelIds.find(InputDeviceMotion::MagneticField::Unbiased) != enabledChannelIds.end() ||
                                    enabledChannelIds.find(InputDeviceMotion::MagneticField::North) != enabledChannelIds.end() ||
                                    enabledChannelIds.find(InputDeviceMotion::Orientation::Current) != enabledChannelIds.end();
        const bool calibratedMagnetometerRequired = enabledChannelIds.find(InputDeviceMotion::MagneticField::Unbiased) != enabledChannelIds.end() ||
                                                    enabledChannelIds.find(InputDeviceMotion::MagneticField::North) != enabledChannelIds.end();
        if (shouldBeActive == m_motionManager.deviceMotionActive &&
            calibratedMagnetometerRequired == m_motionManager.showsDeviceMovementDisplay)
        {
            return;
        }

        // At this point device motion is either:
        // - Not active and we want it active, in which case calling this will do nothing
        // - Active and we want it inactive, in which case this will do so and we'll return below
        // - Active and we still want it active, but we now need to calibrate the magnetometer by
        // showing the device movement display, which won't unless we stop motion device updates.
        [m_motionManager stopDeviceMotionUpdates];
        if (!shouldBeActive)
        {
            return;
        }

        if (!m_motionManager.deviceMotionAvailable)
        {
            return;
        }

        m_motionManager.showsDeviceMovementDisplay = calibratedMagnetometerRequired;

        // For reasons unknown, calibrated magnetic field values are only returned
        // if 'showsDeviceMovementDisplay == true' and device motion updates are
        // started using a reference frame that actually uses the magnetometer.
        const CMAttitudeReferenceFrame referenceFrame = calibratedMagnetometerRequired ?
                                                        CMAttitudeReferenceFrameXArbitraryCorrectedZVertical :
                                                        CMAttitudeReferenceFrameXArbitraryZVertical; // Uses less battery
        [m_motionManager startDeviceMotionUpdatesUsingReferenceFrame : referenceFrame];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector3 InputDeviceMotionIos::MakeVectorRelativeToOrientation(
        float x, float y, float z, UIInterfaceOrientation displayOrientation)
    {
        switch(displayOrientation)
        {
            case UIInterfaceOrientationLandscapeLeft:
            {
                return AZ::Vector3(y, -z, -x);
            }
            break;
            case UIInterfaceOrientationLandscapeRight:
            {
                return AZ::Vector3(-y, -z, x);
            }
            break;
            case UIInterfaceOrientationPortraitUpsideDown:
            {
                return AZ::Vector3(-x, -z, -y);
            }
            break;
            case UIInterfaceOrientationPortrait:
            case UIInterfaceOrientationUnknown:
            default:
            {
                return AZ::Vector3(x, -z, y);
            }
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Quaternion InputDeviceMotionIos::MakeQuaternionRelativeToOrientation(
        float w, float x, float y, float z, UIInterfaceOrientation displayOrientation)
    {
        AZ::Quaternion quaternion = AZ::Quaternion::CreateFromVector3AndValue(AZ::Vector3(x, y, z), w);
        switch(displayOrientation)
        {
            case UIInterfaceOrientationLandscapeLeft:
            {
                quaternion *= AZ::Quaternion::CreateRotationZ(AZ::Constants::HalfPi);
            }
            break;
            case UIInterfaceOrientationLandscapeRight:
            {
                quaternion *= AZ::Quaternion::CreateRotationZ(-AZ::Constants::HalfPi);
            }
            break;
            case UIInterfaceOrientationPortraitUpsideDown:
            {
                quaternion *= AZ::Quaternion::CreateRotationZ(AZ::Constants::Pi);
            }
            break;
            case UIInterfaceOrientationPortrait:
            case UIInterfaceOrientationUnknown:
            default: break;
        }
        return quaternion;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class IosDeviceMotionImplFactory
        : public InputDeviceMotion::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMotion::Implementation> Create(InputDeviceMotion& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceMotionIos>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        m_deviceMotionImplFactory = AZStd::make_unique<IosDeviceMotionImplFactory>();
        AZ::Interface<InputDeviceMotion::ImplementationFactory>::Register(m_deviceMotionImplFactory.get());
    }
} // namespace AzFramework
