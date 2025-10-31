/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(LYSHINE_INTERNAL_UNIT_TEST)

#include <LyShine/Bus/UiCanvasBus.h>
#include <regex>
#include <LyShine/Bus/UiTransform2dBus.h>

namespace
{
    // Helper Functions
    void CreateComponent(AZ::Entity* entity, const AZ::Uuid& componentTypeId)
    {
        entity->Deactivate();
        entity->CreateComponent(componentTypeId);
        entity->Activate();
    }

    AZ::EntityId CreateElementWithTransform2dComponent(UiCanvasInterface* canvas, const char* name) 
    {
        AZ::Entity* testElem = canvas->CreateChildElement(name);
        AZ_Assert(testElem, "Test failed");
        CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
        return testElem->GetId();
    }

    // Helper Function Tests
    
    // Axis Aligned Bounding Box Test
    void TestAABBLogic() 
    {
        // Initialize boxes
        AZ::Vector2 AMin(-1, -1);
        AZ::Vector2 AMax(1, 1);
        AZ::Vector2 BMin(-2, -2);
        AZ::Vector2 BMax(-1, -1);

        // Assert that barely touching corners register as collisions
        AZ_Assert(AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
        BMin.Set(-2, 1);
        BMax.Set(-1, 2);
        AZ_Assert(AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
        BMin.Set(1, 1);
        BMax.Set(2, 2);
        AZ_Assert(AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
        BMin.Set(1, -2);
        BMax.Set(2, -1);
        AZ_Assert(AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
        
        // Assert that things that almost, but do not overlap, do not overlap    
        BMin.Set(-2, 1.1f);
        BMax.Set(-1, 2); 
        AZ_Assert(!AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
        BMin.Set(-2, 1);
        BMax.Set(-1.1f, 2);
        AZ_Assert(!AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
        BMin.Set(1.1f, 1);
        BMax.Set(2, 2);
        AZ_Assert(!AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
        BMin.Set(1, -2);
        BMax.Set(2, -1.1f);
        AZ_Assert(!AxisAlignedBoxesIntersect(AMin, AMax, BMin, BMax), "Test failed");
    }

    // UiTransformBus Tests

    // Test that the Rotation modifying functions operate as intended
    void TestRotation(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:Rotation");

        bool hasRot = true;
        float expectedRot = 0;
        float actualRot = 90;

        // Test that rotation is initialized to the proper defaults
        UiTransformBus::EventResult(actualRot, testElemId, &UiTransformBus::Events::GetZRotation);
        AZ_Assert(actualRot == expectedRot, "Test failed");

        // Test that we aren't registered as having a rotation or scale by default
        UiTransformBus::EventResult(hasRot, testElemId, &UiTransformBus::Events::HasScaleOrRotation);
        AZ_Assert(!hasRot, "Test failed");

        // Test that setting rotation functions properly
        expectedRot = 90;
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, expectedRot);
        UiTransformBus::EventResult(actualRot, testElemId, &UiTransformBus::Events::GetZRotation);
        AZ_Assert(actualRot == expectedRot, "Test failed");

        // Test that we are registered as having a rotation now
        UiTransformBus::EventResult(hasRot, testElemId, &UiTransformBus::Events::HasScaleOrRotation);
        AZ_Assert(hasRot, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }
    
    // Test that the Scale modifying functions operate as intended
    void TestScale(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:Scale");

        AZ::Vector2 expectedScale(1,1);
        AZ::Vector2 actualScale(0,0);
        bool hasScale = true;

        // Test that scale is initialized to the proper defaults
        UiTransformBus::EventResult(actualScale, testElemId, &UiTransformBus::Events::GetScale);
        AZ_Assert(actualScale == expectedScale, "Test failed");

        // Test that we aren't registered as having a rotation or scale by default
        UiTransformBus::EventResult(hasScale, testElemId, &UiTransformBus::Events::HasScaleOrRotation);
        AZ_Assert(!hasScale, "Test failed");

        // Test setting the scale via SetScale
        expectedScale.SetX(5);
        expectedScale.SetY(5);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, expectedScale);
        UiTransformBus::EventResult(actualScale, testElemId, &UiTransformBus::Events::GetScale);
        AZ_Assert(actualScale == expectedScale, "Test failed");

        // Test setting the scale via SetScaleX and SetScaleY
        expectedScale.SetX(8);
        expectedScale.SetY(3);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScaleX, expectedScale.GetX());
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScaleY, expectedScale.GetY());
        UiTransformBus::EventResult(actualScale, testElemId, &UiTransformBus::Events::GetScale);
        AZ_Assert(actualScale == expectedScale, "Test failed");

        // Test retrieving the scale via GetScaleX and GetScaleY
        float getVal;
        expectedScale.SetX(2);
        expectedScale.SetY(9);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, expectedScale);
        UiTransformBus::EventResult(getVal, testElemId, &UiTransformBus::Events::GetScaleX);
        AZ_Assert(getVal == expectedScale.GetX(), "Test failed");
        UiTransformBus::EventResult(getVal, testElemId, &UiTransformBus::Events::GetScaleY);
        AZ_Assert(getVal == expectedScale.GetY(), "Test failed");

        // Test that we are registered as having a scale now
        UiTransformBus::EventResult(hasScale, testElemId, &UiTransformBus::Events::HasScaleOrRotation);
        AZ_Assert(hasScale, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test that the Pivot modifying functions operate as intended
    void TestPivot(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:Pivot");

        AZ::Vector2 expectedPivot(.5f, .5f);
        AZ::Vector2 actualPivot;

        // Test that pivot is initialized to the proper defaults
        UiTransformBus::EventResult(actualPivot, testElemId, &UiTransformBus::Events::GetPivot);
        AZ_Assert(actualPivot == expectedPivot, "Test failed");

        // Test setting the pivot via SetPivot
        expectedPivot.SetX(5);
        expectedPivot.SetY(5);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetPivot, expectedPivot);
        UiTransformBus::EventResult(actualPivot, testElemId, &UiTransformBus::Events::GetPivot);
        AZ_Assert(actualPivot == expectedPivot, "Test failed");

        // Test setting the pivot via SetPivotX and SetPivotY
        expectedPivot.SetX(8);
        expectedPivot.SetY(3);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetPivotX, expectedPivot.GetX());
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetPivotY, expectedPivot.GetY());
        UiTransformBus::EventResult(actualPivot, testElemId, &UiTransformBus::Events::GetPivot);
        AZ_Assert(actualPivot == expectedPivot, "Test failed");

        // Test retrieving the pivot via GetPivotX and GetPivotY
        float getVal;
        expectedPivot.SetX(2);
        expectedPivot.SetY(9);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetPivot, expectedPivot);
        UiTransformBus::EventResult(getVal, testElemId, &UiTransformBus::Events::GetPivotX);
        AZ_Assert(getVal == expectedPivot.GetX(), "Test failed");
        UiTransformBus::EventResult(getVal, testElemId, &UiTransformBus::Events::GetPivotY);
        AZ_Assert(getVal == expectedPivot.GetY(), "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test that the scale to device flag is functioning properly
    void TestScaleToDeviceMode(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");
        
        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:ScaleToDevice");

        AZ::Matrix4x4 active;
        AZ::Matrix4x4 transform;
        AZ::Matrix4x4 transform2;

        // Test that the flag defaults to None
        UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode = UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit;
        UiTransformBus::EventResult(scaleToDeviceMode, testElemId, &UiTransformBus::Events::GetScaleToDeviceMode);
        AZ_Assert(scaleToDeviceMode == UiTransformInterface::ScaleToDeviceMode::None, "Test failed");

        // Test that we aren't registered as having a rotation or scale by default
        bool hasScaleOrRotation = true;
        UiTransformBus::EventResult(hasScaleOrRotation, testElemId, &UiTransformBus::Events::HasScaleOrRotation);
        AZ_Assert(!hasScaleOrRotation, "Test failed");

        // Test that scaling to the device modifies the transform
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScaleToDeviceMode, UiTransformInterface::ScaleToDeviceMode::None);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform);
        UiTransformBus::Event(
            testElemId, &UiTransformBus::Events::SetScaleToDeviceMode, UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
        
        // Resize the canvas to change the DeviceScale
        canvas->SetTargetCanvasSize(true, AZ::Vector2(3, 3));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform2);
        AZ_Assert(transform != transform2, "Test failed");

        // Test that setting it to None when it is already None, does not set it to UniformScaleToFit.
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScaleToDeviceMode, UiTransformInterface::ScaleToDeviceMode::None);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScaleToDeviceMode, UiTransformInterface::ScaleToDeviceMode::None);
        UiTransformBus::EventResult(scaleToDeviceMode, testElemId, &UiTransformBus::Events::GetScaleToDeviceMode);
        AZ_Assert(scaleToDeviceMode == UiTransformInterface::ScaleToDeviceMode::None, "Test failed");

        // Check that the flag is actually disabled by checking the transform
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, active);
        AZ_Assert(active == transform, "Test failed");

        // Test that setting it to UniformScaleToFit when it is None, sets it to UniformScaleToFit
        UiTransformBus::Event(
            testElemId, &UiTransformBus::Events::SetScaleToDeviceMode, UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
        UiTransformBus::EventResult(scaleToDeviceMode, testElemId, &UiTransformBus::Events::GetScaleToDeviceMode);
        AZ_Assert(scaleToDeviceMode == UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit, "Test failed");

        // Check that the flag is actually working by checking the transform
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, active);
        AZ_Assert(active == transform2, "Test failed");

        // Test that we are registered as having a scale by now
        UiTransformBus::EventResult(hasScaleOrRotation, testElemId, &UiTransformBus::Events::HasScaleOrRotation);
        AZ_Assert(hasScaleOrRotation, "Test failed");

        // Test that setting it to UniformScaleToFit when it is UniformScaleToFit, does not set it to None
        UiTransformBus::Event(
            testElemId, &UiTransformBus::Events::SetScaleToDeviceMode, UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
        UiTransformBus::EventResult(scaleToDeviceMode, testElemId, &UiTransformBus::Events::GetScaleToDeviceMode);
        AZ_Assert(scaleToDeviceMode == UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit, "Test failed");

        // Check that the flag is actually enabled by checking the transform
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, active);
        AZ_Assert(active == transform2, "Test failed");

        // Test that setting it to None when it is UniformScaleToFit, properly sets it to None.
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScaleToDeviceMode, UiTransformInterface::ScaleToDeviceMode::None);
        UiTransformBus::EventResult(scaleToDeviceMode, testElemId, &UiTransformBus::Events::GetScaleToDeviceMode);
        AZ_Assert(scaleToDeviceMode == UiTransformInterface::ScaleToDeviceMode::None, "Test failed");

        // Check that the flag is actually disabled by checking the transform
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, active);
        AZ_Assert(active == transform, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test that ViewportSpace Transforms operate properly
    void TestViewportSpaceTransforms(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:ViewportSpaceTransform");

        // Set up a Canvas to Viewport Matrix
        AZ::Matrix4x4 updatedMatrix = AZ::Matrix4x4::CreateScale(AZ::Vector3(5, 5, 1));
        updatedMatrix.SetTranslation(AZ::Vector3(5, 5, 5));
        UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::SetCanvasToViewportMatrix, updatedMatrix);
        canvas->ReinitializeElements();

        AZ::Matrix4x4 transformToVp;
        AZ::Matrix4x4 transformFromVp;

        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetTransformToViewport, transformToVp);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetTransformFromViewport, transformFromVp);
        AZ_Assert(transformFromVp.IsClose(transformToVp.GetInverseFull()), "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test that CanvasSpace Transforms operate properly
    void TestCanvasSpaceTransforms(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:CanvasSpaceTransform");

        AZ::Matrix4x4 transformToCanvas;
        AZ::Matrix4x4 transformFromCanvas;

        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetTransformToCanvasSpace, transformToCanvas);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetTransformFromCanvasSpace, transformFromCanvas);
        AZ_Assert(transformFromCanvas.IsClose(transformToCanvas.GetInverseFull()), "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test No rotate, no scale space
    void TestCanvasSpaceNoScaleNoRot(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:NoScaleNoRot");

        UiTransformInterface::Rect rectangle;
        UiTransformInterface::RectPoints rectanglePoints;
        AZ::Vector2 canvasSpaceSize;
        AZ::Vector2 canvasSpacePivot;

        UiTransformInterface::Rect rectangleTest;
        UiTransformInterface::RectPoints rectanglePointsTest;
        AZ::Vector2 canvasSpaceSizeTest;
        AZ::Vector2 canvasSpacePivotTest;

        // Get Initial values
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, rectangle);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, rectanglePoints);
        UiTransformBus::EventResult(canvasSpaceSize, testElemId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);
        UiTransformBus::EventResult(canvasSpacePivot, testElemId, &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);

        // Rotate and scale and see if the values change
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 76.f);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(50, 50));

        // Get Post-transform values
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, rectangleTest);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, rectanglePointsTest);
        UiTransformBus::EventResult(canvasSpaceSizeTest, testElemId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);
        UiTransformBus::EventResult(canvasSpacePivotTest, testElemId, &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);
         
        // See if the values change
        AZ_Assert(rectangle == rectangleTest, "Test failed");
        AZ_Assert(rectanglePoints.TopLeft() == rectanglePointsTest.TopLeft(), "Test failed");
        AZ_Assert(rectanglePoints.TopRight() == rectanglePointsTest.TopRight(), "Test failed");
        AZ_Assert(rectanglePoints.BottomLeft() == rectanglePointsTest.BottomLeft(), "Test failed");
        AZ_Assert(rectanglePoints.BottomRight() == rectanglePointsTest.BottomRight(), "Test failed");
        AZ_Assert(canvasSpaceSize == canvasSpaceSizeTest, "Test failed");
        AZ_Assert(canvasSpacePivot == canvasSpacePivotTest, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test for Local Transform Accessors
    void TestLocalTransform(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:LocalTransform");

        const float testScale = .2f;
        const float sinOf45 = .7071f;
        AZ::Matrix4x4 transform;

        // Check the default value
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform);
        AZ_Assert(transform == AZ::Matrix4x4::CreateIdentity(), "Test failed");

        // Check Scale transform values
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(testScale, testScale));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform);
        AZ_Assert(AZ::IsClose(transform.RetrieveScale().GetX(), testScale, AZ::Constants::FloatEpsilon), "Test failed");
        AZ_Assert(AZ::IsClose(transform.RetrieveScale().GetY(), testScale, AZ::Constants::FloatEpsilon), "Test failed");
        AZ_Assert(AZ::IsClose(transform.RetrieveScale().GetZ(), 1, AZ::Constants::FloatEpsilon), "Test failed");

        // No translational data should be present
        AZ_Assert(AZ::Vector3(0, 0, 0) * transform == AZ::Vector3(0, 0, 0), "Test failed");

        // Check Rotation values
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(1.f, 1.f));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 90.f);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform);
        AZ_Assert(AZ::IsClose((AZ::Vector3(1.f, 0, 0) * transform).GetY(), -1.f, AZ::Constants::FloatEpsilon), "Test failed");
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 45.f);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform);
        AZ_Assert(AZ::IsClose((AZ::Vector3(1.f, 0, 0) * transform).GetY(), -sinOf45, .001f ), "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test for Local Transform Accessors
    void TestLocalInverseTransform(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:LocalTransform");

        const float testScale = .2f;
        const float inverseTestScale = 1 / testScale;
        const float sinOf45 = .7071f;
        AZ::Matrix4x4 transform;

        // Check the default value
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform);
        AZ_Assert(transform == AZ::Matrix4x4::CreateIdentity(), "Test failed");

        // Check Scale transform values
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(testScale, testScale));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalInverseTransform, transform);
        AZ_Assert(AZ::IsClose(transform.RetrieveScale().GetX(), inverseTestScale, AZ::Constants::FloatEpsilon), "Test failed");
        AZ_Assert(AZ::IsClose(transform.RetrieveScale().GetY(), inverseTestScale, AZ::Constants::FloatEpsilon), "Test failed");
        AZ_Assert(AZ::IsClose(transform.RetrieveScale().GetZ(), 1, AZ::Constants::FloatEpsilon), "Test failed");

        // No translational data should be present
        AZ_Assert(AZ::Vector3(0, 0, 0) * transform == AZ::Vector3(0, 0, 0), "Test failed");

        // Check Rotation values
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(1.f, 1.f));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 90.f);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalInverseTransform, transform);
        AZ_Assert(AZ::IsClose((AZ::Vector3(1.f, 0, 0) * transform).GetY(), 1.f, AZ::Constants::FloatEpsilon), "Test failed");
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 45.f);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalInverseTransform, transform);
        AZ_Assert(AZ::IsClose((AZ::Vector3(1.f, 0, 0) * transform).GetY(), sinOf45, .001f), "Test failed");

        //Check against normal transform
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 90.f);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(9, 5));
        AZ::Matrix4x4 transform2;
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalInverseTransform, transform);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetLocalTransform, transform2);
        AZ::Vector3 before(1, 0, 0);
        AZ::Vector3 after = before * transform * transform2;
        AZ_Assert(after.IsClose(before, AZ::Constants::FloatEpsilon), "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test local positioning methods
    void TestLocalPositioning(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:LocalPositioning");

        AZ::Vector2 expectedLocalPos(0, 0);
        AZ::Vector2 actualLocalPos;

        // Test that local position is initialized to the proper defaults
        UiTransformBus::EventResult(actualLocalPos, testElemId, &UiTransformBus::Events::GetLocalPosition);
        AZ_Assert(actualLocalPos == expectedLocalPos, "Test failed");

        // Test setting the pivot via SetLocalPosition
        expectedLocalPos.SetX(5);
        expectedLocalPos.SetY(5);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetLocalPosition, expectedLocalPos);
        UiTransformBus::EventResult(actualLocalPos, testElemId, &UiTransformBus::Events::GetLocalPosition);
        AZ_Assert(actualLocalPos == expectedLocalPos, "Test failed");

        // Test setting the local position via SetLocalPositionX and SetLocalPositionY
        expectedLocalPos.SetX(8);
        expectedLocalPos.SetY(3);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetLocalPositionX, expectedLocalPos.GetX());
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetLocalPositionY, expectedLocalPos.GetY());
        UiTransformBus::EventResult(actualLocalPos, testElemId, &UiTransformBus::Events::GetLocalPosition);
        AZ_Assert(actualLocalPos == expectedLocalPos, "Test failed");

        // Test retrieving the local position via GetLocalPositionX and GetLocalPositionY
        float getVal;
        expectedLocalPos.SetX(2);
        expectedLocalPos.SetY(9);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetLocalPosition, expectedLocalPos);
        UiTransformBus::EventResult(getVal, testElemId, &UiTransformBus::Events::GetLocalPositionX);
        AZ_Assert(getVal == expectedLocalPos.GetX(), "Test failed");
        UiTransformBus::EventResult(getVal, testElemId, &UiTransformBus::Events::GetLocalPositionY);
        AZ_Assert(getVal == expectedLocalPos.GetY(), "Test failed");

        // Test offset by
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetLocalPosition, AZ::Vector2(0,0));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::MoveLocalPositionBy, expectedLocalPos);
        UiTransformBus::EventResult(actualLocalPos, testElemId, &UiTransformBus::Events::GetLocalPosition);
        AZ_Assert(actualLocalPos == expectedLocalPos, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test viewport positioning
    void TestViewportPositioning(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:ViewportPositioning");

        AZ::Vector2 expectedViewportPos(0, 0);
        AZ::Vector2 actualViewportPos;

        // Test setting the viewport position via SetViewportPosition
        expectedViewportPos.SetX(5);
        expectedViewportPos.SetY(5);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetViewportPosition, expectedViewportPos);
        UiTransformBus::EventResult(actualViewportPos, testElemId, &UiTransformBus::Events::GetViewportPosition);
        AZ_Assert(actualViewportPos == expectedViewportPos, "Test failed");

        // Test offset by
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetViewportPosition, AZ::Vector2(0, 0));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::MoveViewportPositionBy, expectedViewportPos);
        UiTransformBus::EventResult(actualViewportPos, testElemId, &UiTransformBus::Events::GetViewportPosition);
        AZ_Assert(actualViewportPos == expectedViewportPos, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test canvas positioning
    void TestCanvasPositioning(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:CanvasPositioning");

        AZ::Vector2 expectedCanvasPos(0, 0);
        AZ::Vector2 actualCanvasPos;

        // Test setting the canvas position via SetCanvasPosition
        expectedCanvasPos.SetX(5);
        expectedCanvasPos.SetY(5);
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetCanvasPosition, expectedCanvasPos);
        UiTransformBus::EventResult(actualCanvasPos, testElemId, &UiTransformBus::Events::GetCanvasPosition);
        AZ_Assert(actualCanvasPos == expectedCanvasPos, "Test failed");

        // Test offset by
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetCanvasPosition, AZ::Vector2(0, 0));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::MoveCanvasPositionBy, expectedCanvasPos);
        UiTransformBus::EventResult(actualCanvasPos, testElemId, &UiTransformBus::Events::GetCanvasPosition);
        AZ_Assert(actualCanvasPos == expectedCanvasPos, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test IsPointInRect
    void TestIsPointInRect(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:IsPointInRect");

        UiTransformInterface::Rect bounds;
        // Points in list A should pass the normal overlap test, but fail the scale and rotation tests
        std::vector<AZ::Vector2> pointsA(4);
        // Points in list B should fail the normal overlap test, but pass the scale and rotation tests
        std::vector<AZ::Vector2> pointsB(4);
        bool result = false;

        // Get bounds without rotation or scale
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, bounds);

        // Set up points
        pointsA[0].Set(bounds.left, bounds.top);
        pointsA[1].Set(bounds.left, bounds.bottom);
        pointsA[2].Set(bounds.right, bounds.top);
        pointsA[3].Set(bounds.right, bounds.bottom);

        pointsB[0].Set(bounds.GetCenterX(), bounds.top - .1f);
        pointsB[1].Set(bounds.GetCenterX(), bounds.bottom + .1f);
        pointsB[2].Set(bounds.left - .1f, bounds.GetCenterY());
        pointsB[3].Set(bounds.right + .1f, bounds.GetCenterY());

        // Test positive cases
        for (size_t i = 0; i < pointsA.size(); i++)
        {
            UiTransformBus::EventResult(result, testElemId, &UiTransformBus::Events::IsPointInRect, pointsA[i]);
            AZ_Assert(result, "Test failed");
        }

        // Test negative cases
        for (size_t i = 0; i < pointsB.size(); i++)
        {
            UiTransformBus::EventResult(result, testElemId, &UiTransformBus::Events::IsPointInRect, pointsB[i]);
            AZ_Assert(!result, "Test failed");
        }

        // Test cases that would be positive cases, but aren't due to a scale
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(.5f, .5f));

        for (size_t i = 0; i < pointsA.size(); i++)
        {
            UiTransformBus::EventResult(result, testElemId, &UiTransformBus::Events::IsPointInRect, pointsA[i]);
            AZ_Assert(!result, "Test failed");
        }

        // Test cases that would be negative cases, but aren't due to a scale
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(1.1f, 1.1f));

        for (size_t i = 0; i < pointsB.size(); i++)
        {
            UiTransformBus::EventResult(result, testElemId, &UiTransformBus::Events::IsPointInRect, pointsB[i]);
            AZ_Assert(result, "Test failed");
        }

        // Test cases that would be positive cases, but aren't due to a rotation
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(1, 1));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 45.f);

        for (size_t i = 0; i < pointsA.size(); i++)
        {
            UiTransformBus::EventResult(result, testElemId, &UiTransformBus::Events::IsPointInRect, pointsA[i]);
            AZ_Assert(!result, "Test failed");
        }

        // Test cases that would be negative cases, but aren't due to a rotation
        for (size_t i = 0; i < pointsB.size(); i++)
        {
            UiTransformBus::EventResult(result, testElemId, &UiTransformBus::Events::IsPointInRect, pointsB[i]);
            AZ_Assert(result, "Test failed");
        }

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test BoundsAreOverlappingRect

    void TestBoundsAreOverlappingRect(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:BoundsAreOverlappingRect");

        UiTransformInterface::Rect objBounds;
        // Bounds in list A should pass the normal overlap test, but fail the scale and rotation tests
        std::vector<UiTransformInterface::Rect> boundsA(4);
        // Bounds in list B should fail the normal overlap test, but pass the scale and rotation tests
        std::vector<UiTransformInterface::Rect> boundsB(4);
        bool result = false;

        // Get bounds without rotation or scale
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, objBounds);

        // Set up bounds
        boundsA[0].Set(objBounds.left - 1, objBounds.left, objBounds.top - 1, objBounds.top);
        boundsA[1].Set(objBounds.left - 1, objBounds.left, objBounds.bottom, objBounds.bottom + 1);
        boundsA[2].Set(objBounds.right, objBounds.right + 1, objBounds.top - 1, objBounds.top);
        boundsA[3].Set(objBounds.right, objBounds.right + 1, objBounds.bottom, objBounds.bottom + 1);

        boundsB[0].Set(objBounds.GetCenterX(), objBounds.GetCenterX(), objBounds.top - 1, objBounds.top - .1f);
        boundsB[1].Set(objBounds.GetCenterX(), objBounds.GetCenterX(), objBounds.bottom + .1f, objBounds.bottom + 1);
        boundsB[2].Set(objBounds.left - 1, objBounds.left - .1f, objBounds.GetCenterY(), objBounds.GetCenterY());
        boundsB[3].Set(objBounds.right + .1f, objBounds.right + 1, objBounds.GetCenterY(), objBounds.GetCenterY());

        // Test positive cases
        for (size_t i = 0; i < boundsA.size(); i++)
        {
            UiTransformBus::EventResult(
                result,
                testElemId,
                &UiTransformBus::Events::BoundsAreOverlappingRect,
                AZ::Vector2(boundsA[i].left, boundsA[i].top),
                AZ::Vector2(boundsA[i].right, boundsA[i].bottom));
            AZ_Assert(result, "Test failed");
        }
        
        // Test negative cases
        for (size_t i = 0; i < boundsB.size(); i++)
        {
            UiTransformBus::EventResult(
                result,
                testElemId,
                &UiTransformBus::Events::BoundsAreOverlappingRect,
                AZ::Vector2(boundsB[i].left, boundsB[i].top),
                AZ::Vector2(boundsB[i].right, boundsB[i].bottom));
            AZ_Assert(!result, "Test failed");
        }

        // Test cases that would be positive cases, but aren't due to a scale
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(.5f, .5f));

        for (size_t i = 0; i < boundsA.size(); i++)
        {
            UiTransformBus::EventResult(
                result,
                testElemId,
                &UiTransformBus::Events::BoundsAreOverlappingRect,
                AZ::Vector2(boundsA[i].left, boundsA[i].top),
                AZ::Vector2(boundsA[i].right, boundsA[i].bottom));
            AZ_Assert(!result, "Test failed");
        }

        // Test cases that would be negative cases, but aren't due to a scale
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(1.1f, 1.1f));

        for (size_t i = 0; i < boundsB.size(); i++)
        {
            UiTransformBus::EventResult(
                result,
                testElemId,
                &UiTransformBus::Events::BoundsAreOverlappingRect,
                AZ::Vector2(boundsB[i].left, boundsB[i].top),
                AZ::Vector2(boundsB[i].right, boundsB[i].bottom));
            AZ_Assert(result, "Test failed");
        }

        // Test cases that would be positive cases, but aren't due to a rotation
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetScale, AZ::Vector2(1, 1));
        UiTransformBus::Event(testElemId, &UiTransformBus::Events::SetZRotation, 45.f);

        for (size_t i = 0; i < boundsA.size(); i++)
        {
            UiTransformBus::EventResult(
                result,
                testElemId,
                &UiTransformBus::Events::BoundsAreOverlappingRect,
                AZ::Vector2(boundsA[i].left, boundsA[i].top),
                AZ::Vector2(boundsA[i].right, boundsA[i].bottom));
            AZ_Assert(!result, "Test failed");
        }

        // Test cases that would be negative cases, but aren't due to a rotation
        for (size_t i = 0; i < boundsB.size(); i++)
        {
            UiTransformBus::EventResult(
                result,
                testElemId,
                &UiTransformBus::Events::BoundsAreOverlappingRect,
                AZ::Vector2(boundsB[i].left, boundsB[i].top),
                AZ::Vector2(boundsB[i].right, boundsB[i].bottom));
            AZ_Assert(result, "Test failed");
        }

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // UITransform2dBus Tests

    // Test the anchor pushing parameter
    void TestAnchorsPush(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:AnchorPush");

        UiTransform2dInterface::Anchors actualAnchors;
        UiTransform2dInterface::Anchors expectedAnchors(.5f, .5f, .5f, .5f);

        // Test for expected defaults
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");
        
        // Test Allow Push false
        actualAnchors.m_bottom--;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, false);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        actualAnchors.m_top++;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, false);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        actualAnchors.m_left++;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, false);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        actualAnchors.m_right--;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, false);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        // Test Allow Push true
        actualAnchors.m_bottom--;
        expectedAnchors.m_bottom--;
        expectedAnchors.m_top--;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, true);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        actualAnchors.m_top++;
        expectedAnchors.m_bottom++;
        expectedAnchors.m_top++; 
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, true);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        actualAnchors.m_left++;
        expectedAnchors.m_left++;
        expectedAnchors.m_right++;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, true);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        actualAnchors.m_right--;
        expectedAnchors.m_left--;
        expectedAnchors.m_right--; 
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, actualAnchors, false, true);
        UiTransform2dBus::EventResult(actualAnchors, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(actualAnchors == expectedAnchors, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }

    // Test the anchor adjusting offset parameter functions properly
    void TestAnchorsAdjustOffset(CLyShine* lyShine) 
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:AnchorsAdjustOffset");

        AZ::Vector2 parentSize(canvas->GetCanvasSize());
        UiTransform2dInterface::Anchors testAnch;
        UiTransform2dInterface::Offsets expectedOffsets(-50, -50, 50, 50);
        UiTransform2dInterface::Offsets actualOffsets;

        // Test for expected defaults
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == .5f && testAnch.m_bottom == .5f && testAnch.m_left == .5f && testAnch.m_right == .5f, "Test failed");
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");
        
        // Test Offset values properly don't change
        testAnch.m_bottom++;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, false, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == .5f && testAnch.m_bottom == 1.5f && testAnch.m_left == .5f && testAnch.m_right == .5f, "Test failed");
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        testAnch.m_top--;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, false, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == -.5f && testAnch.m_bottom == 1.5f && testAnch.m_left == .5f && testAnch.m_right == .5f, "Test failed");
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        testAnch.m_left--;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, false, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == -.5f && testAnch.m_bottom == 1.5f && testAnch.m_left == -.5f && testAnch.m_right == .5f, "Test failed");
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        testAnch.m_right++;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, false, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == -.5f && testAnch.m_bottom == 1.5f && testAnch.m_left == -.5f && testAnch.m_right == 1.5f, "Test failed");
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        // Reset the data
        testAnch.m_bottom = .5f;
        testAnch.m_top = .5f;
        testAnch.m_left = .5f;
        testAnch.m_right = .5f;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, false, false);
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        // Test Offset values properly change
        testAnch.m_bottom++;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, true, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == .5f && testAnch.m_bottom == 1.5f && testAnch.m_left == .5f && testAnch.m_right == .5f, "Test failed");
        expectedOffsets.m_bottom -= parentSize.GetY();
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        testAnch.m_top--;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, true, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == -.5f && testAnch.m_bottom == 1.5f && testAnch.m_left == .5f && testAnch.m_right == .5f, "Test failed");
        expectedOffsets.m_top += parentSize.GetY();
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        testAnch.m_left--;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, true, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == -.5f && testAnch.m_bottom == 1.5f && testAnch.m_left == -.5f && testAnch.m_right == .5f, "Test failed");
        expectedOffsets.m_left += parentSize.GetX();
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        testAnch.m_right++;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetAnchors, testAnch, true, false);
        UiTransform2dBus::EventResult(testAnch, testElemId, &UiTransform2dBus::Events::GetAnchors);
        AZ_Assert(testAnch.m_top == -.5f && testAnch.m_bottom == 1.5f && testAnch.m_left == -.5f && testAnch.m_right == 1.5f, "Test failed");
        expectedOffsets.m_right -= parentSize.GetX();
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }


    void TestOffsets(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:Offsets");

        UiTransform2dInterface::Offsets expectedOffsets(-50, -50, 50, 50);
        UiTransform2dInterface::Offsets actualOffsets;

        // Test for expected defaults
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        // Test setting the offset via SetOffsets for all types of test cases
        actualOffsets.m_bottom = -100;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetOffsets, actualOffsets);
        expectedOffsets.m_bottom = -50;
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");
        
        actualOffsets.m_top = 100;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetOffsets, actualOffsets);
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");
        
        actualOffsets.m_bottom = -100;
        actualOffsets.m_top = 100;
        expectedOffsets.m_bottom = 0;
        expectedOffsets.m_top = 0;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetOffsets, actualOffsets);
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        actualOffsets.m_right = -100;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetOffsets, actualOffsets);
        expectedOffsets.m_right = -50;
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        actualOffsets.m_left = 100;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetOffsets, actualOffsets);
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        actualOffsets.m_right = -100;
        actualOffsets.m_left = 100;
        expectedOffsets.m_right = 0;
        expectedOffsets.m_left = 0;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetOffsets, actualOffsets);
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        expectedOffsets.m_bottom = 66;
        expectedOffsets.m_top = -5;
        expectedOffsets.m_right = 83;
        expectedOffsets.m_left = -99;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetOffsets, expectedOffsets);
        UiTransform2dBus::EventResult(actualOffsets, testElemId, &UiTransform2dBus::Events::GetOffsets);
        AZ_Assert(actualOffsets == expectedOffsets, "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }
    
    // Test local size
    void TestLocalSizeParameters(CLyShine* lyShine)
    {
        AZ::EntityId canvasEntityId = lyShine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::EntityId testElemId = CreateElementWithTransform2dComponent(canvas, "UiTransfrom2DTestElement:LocalSize");

        float expectedWidth = 100;
        float actualWidth = 1;
        float expectedHeight = 100;
        float actualHeight = 1;

        // Test for expected defaults
        UiTransform2dBus::EventResult(actualWidth, testElemId, &UiTransform2dBus::Events::GetLocalWidth);
        AZ_Assert(actualWidth == expectedWidth, "Test failed"); 
        UiTransform2dBus::EventResult(actualHeight, testElemId, &UiTransform2dBus::Events::GetLocalHeight);
        AZ_Assert(actualHeight == expectedHeight, "Test failed");

        // Test that setters function
        expectedHeight = 77;
        expectedWidth = 33;
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetLocalWidth, expectedWidth);
        UiTransform2dBus::EventResult(actualWidth, testElemId, &UiTransform2dBus::Events::GetLocalWidth);
        AZ_Assert(AZ::IsClose(actualWidth, expectedWidth, AZ::Constants::FloatEpsilon), "Test failed");
        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetLocalHeight, expectedHeight);
        UiTransform2dBus::EventResult(actualHeight, testElemId, &UiTransform2dBus::Events::GetLocalHeight);
        AZ_Assert(AZ::IsClose(actualHeight, expectedHeight, AZ::Constants::FloatEpsilon ), "Test failed");

        UiTransform2dBus::Event(testElemId, &UiTransform2dBus::Events::SetLocalWidth, expectedWidth);

        // Test that when there isn't a fixed width the functions don't give non-zero return values
        UiTransform2dBus::Event(
            testElemId, &UiTransform2dBus::Events::SetAnchors, UiTransform2dInterface::Anchors(0, 0, 1, 1), false, false);
        UiTransform2dBus::EventResult(actualWidth, testElemId, &UiTransform2dBus::Events::GetLocalWidth);
        AZ_Assert(AZ::IsClose(actualWidth, 0, AZ::Constants::FloatEpsilon ), "Test failed");
        UiTransform2dBus::EventResult(actualHeight, testElemId, &UiTransform2dBus::Events::GetLocalHeight);
        AZ_Assert(AZ::IsClose(actualHeight, 0, AZ::Constants::FloatEpsilon ), "Test failed");

        UiTransform2dBus::Event(
            testElemId, &UiTransform2dBus::Events::SetAnchors, UiTransform2dInterface::Anchors(0, 1, 1, 1), false, false);
        UiTransform2dBus::EventResult(actualWidth, testElemId, &UiTransform2dBus::Events::GetLocalWidth);
        AZ_Assert(AZ::IsClose(actualWidth, 0, AZ::Constants::FloatEpsilon ), "Test failed");
        UiTransform2dBus::EventResult(actualHeight, testElemId, &UiTransform2dBus::Events::GetLocalHeight);
        AZ_Assert(AZ::IsClose(actualHeight, expectedHeight, AZ::Constants::FloatEpsilon ), "Test failed");

        UiTransform2dBus::Event(
            testElemId, &UiTransform2dBus::Events::SetAnchors, UiTransform2dInterface::Anchors(1, 0, 1, 1), false, false);
        UiTransform2dBus::EventResult(actualWidth, testElemId, &UiTransform2dBus::Events::GetLocalWidth);
        AZ_Assert(AZ::IsClose(actualWidth, expectedWidth, AZ::Constants::FloatEpsilon ), "Test failed");
        UiTransform2dBus::EventResult(actualHeight, testElemId, &UiTransform2dBus::Events::GetLocalHeight);
        AZ_Assert(AZ::IsClose(actualHeight, 0, AZ::Constants::FloatEpsilon ), "Test failed");

        lyShine->ReleaseCanvas(canvasEntityId, false);
    }
}

void UiTransform2dComponent::UnitTest(CLyShine* lyShine, IConsoleCmdArgs* /* cmdArgs */)
{
    // Helper function tests
    TestAABBLogic();

    // UiTransformBus tests
    TestRotation(lyShine);
    TestScale(lyShine);
    TestPivot(lyShine);
    TestScaleToDeviceMode(lyShine);
    TestViewportSpaceTransforms(lyShine);
    TestCanvasSpaceTransforms(lyShine);
    TestCanvasSpaceNoScaleNoRot(lyShine);
    TestLocalTransform(lyShine);
    TestLocalInverseTransform(lyShine);
    TestLocalPositioning(lyShine);
    TestViewportPositioning(lyShine);
    TestCanvasPositioning(lyShine);
    TestIsPointInRect(lyShine);
    TestBoundsAreOverlappingRect(lyShine);

    // UiTransform2dBus tests
    TestAnchorsPush(lyShine);
    TestAnchorsAdjustOffset(lyShine);
    TestOffsets(lyShine);
    TestLocalSizeParameters(lyShine);
}

#endif
