/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiNavigationHelpers.h"

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiNavigationBus.h>
#include <LyShine/Bus/UiInteractableBus.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

namespace UiNavigationHelpers
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    Command MapInputChannelIdToUiNavigationCommand(const AzFramework::InputChannelId& inputChannelId,
        AzFramework::ModifierKeyMask activeModifierKeys)
    {
        if (inputChannelId == AzFramework::InputDeviceGamepad::Button::DU ||
            inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickDirection::LU ||
            inputChannelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp)
        {
            return Command::Up;
        }

        if (inputChannelId == AzFramework::InputDeviceGamepad::Button::DD ||
            inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickDirection::LD ||
            inputChannelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown)
        {
            return Command::Down;
        }

        if (inputChannelId == AzFramework::InputDeviceGamepad::Button::DL ||
            inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickDirection::LL ||
            inputChannelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft)
        {
            return Command::Left;
        }

        if (inputChannelId == AzFramework::InputDeviceGamepad::Button::DR ||
            inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickDirection::LR ||
            inputChannelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight)
        {
            return Command::Right;
        }

        bool enterPressed = inputChannelId == AzFramework::InputDeviceKeyboard::Key::EditEnter ||
            inputChannelId == AzFramework::InputDeviceVirtualKeyboard::Command::EditEnter;

        bool shiftModifierPressed = (static_cast<int>(activeModifierKeys) & static_cast<int>(AzFramework::ModifierKeyMask::ShiftAny)) != 0;

        if (inputChannelId == AzFramework::InputDeviceGamepad::Button::A ||
            (enterPressed && !shiftModifierPressed))
        {
            return Command::Enter;
        }

        if (inputChannelId == AzFramework::InputDeviceGamepad::Button::B ||
            inputChannelId == AzFramework::InputDeviceKeyboard::Key::Escape ||
            (enterPressed && shiftModifierPressed))
        {
            return Command::Back;
        }

        if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::NavigationEnd)
        {
            return Command::NavEnd;
        }

        if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::NavigationHome)
        {
            return Command::NavHome;
        }

        return Command::Unknown;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId GetNextElement(AZ::EntityId curEntityId, Command command,
        const LyShine::EntityArray& navigableElements, AZ::EntityId defaultEntityId,
        ValidationFunction isValidResult, AZ::EntityId parentElement)
    {
        AZ::EntityId nextEntityId;
        bool found = false;
        do
        {
            nextEntityId.SetInvalid();
            UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
            UiNavigationBus::EventResult(navigationMode, curEntityId, &UiNavigationBus::Events::GetNavigationMode);
            if (navigationMode == UiNavigationInterface::NavigationMode::Custom)
            {
                // Ask the current interactable what the next interactable should be
                nextEntityId = FollowCustomLink(curEntityId, command);

                if (nextEntityId.IsValid())
                {
                    // Skip over elements that are not valid
                    if (isValidResult(nextEntityId))
                    {
                        found = true;
                    }
                    else
                    {
                        curEntityId = nextEntityId;
                    }
                }
                else
                {
                    found = true;
                }
            }
            else if (navigationMode == UiNavigationInterface::NavigationMode::Automatic)
            {
                nextEntityId = SearchForNextElement(curEntityId, command, navigableElements, parentElement);
                found = true;
            }
            else
            {
                // If navigationMode is None we should never get here via keyboard navigation
                // and we may not be able to get to other elements from here (e.g. this could be
                // a full screen button in the background). So go to the passed in default.
                nextEntityId = defaultEntityId;
                found = true;
            }
        } while (!found);

        return nextEntityId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId SearchForNextElement(AZ::EntityId curElement, Command command,
        const LyShine::EntityArray& navigableElements, AZ::EntityId parentElement)
    {
        // Check if the current element is a descendant of the parent of the navigable elements.
        // If it isn't a descendant, then priority is given to the navigable elements
        // that are visible within their parent's bounds
        bool isCurElementDescendantOfParentElement = false;
        if (parentElement.IsValid())
        {
            UiElementBus::EventResult(isCurElementDescendantOfParentElement, curElement, &UiElementBus::Events::IsAncestor, parentElement);
        }

        UiTransformInterface::Rect parentRect;
        parentRect.Set(0.0f, 0.0f, 0.0f, 0.0f);

        AZ::Matrix4x4 parentTransformFromViewport;
        if (parentElement.IsValid() && !isCurElementDescendantOfParentElement)
        {
            UiTransformBus::Event(parentElement, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);
            UiTransformBus::Event(parentElement, &UiTransformBus::Events::GetTransformFromViewport, parentTransformFromViewport);
        }

        UiTransformInterface::RectPoints srcPoints;
        UiTransformBus::Event(curElement, &UiTransformBus::Events::GetViewportSpacePoints, srcPoints);
        AZ::Vector2 srcCenter = srcPoints.GetCenter();

        // Go through the navigable elements and find the closest element to the current hover interactable
        float shortestDist = FLT_MAX;
        float shortestCenterToCenterDist = FLT_MAX;
        AZ::EntityId closestElement;
        float shortestOutsideDist = FLT_MAX;
        float shortestOutsideCenterToCenterDist = FLT_MAX;
        AZ::EntityId closestOutsideElement;
        for (auto navigableElement : navigableElements)
        {
            UiTransformInterface::RectPoints destPoints;
            UiTransformBus::Event(navigableElement->GetId(), &UiTransformBus::Events::GetViewportSpacePoints, destPoints);
            AZ::Vector2 destCenter = destPoints.GetCenter();

            bool correctDirection = false;
            if (command == Command::Up)
            {
                correctDirection = destCenter.GetY() < srcPoints.GetAxisAlignedTopLeft().GetY();
            }
            else if (command == Command::Down)
            {
                correctDirection = destCenter.GetY() > srcPoints.GetAxisAlignedBottomLeft().GetY();
            }
            else if (command == Command::Left)
            {
                correctDirection = destCenter.GetX() < srcPoints.GetAxisAlignedTopLeft().GetX();
            }
            else if (command == Command::Right)
            {
                correctDirection = destCenter.GetX() > srcPoints.GetAxisAlignedTopRight().GetX();
            }

            if (correctDirection)
            {
                // Calculate an overlap value from 0 to 1
                float overlapValue = 0.0f;
                if (command == Command::Up || command == Command::Down)
                {
                    float srcLeft = srcPoints.GetAxisAlignedTopLeft().GetX();
                    float srcRight = srcPoints.GetAxisAlignedTopRight().GetX();
                    float destLeft = destPoints.GetAxisAlignedTopLeft().GetX();
                    float destRight = destPoints.GetAxisAlignedTopRight().GetX();

                    if ((srcLeft <= destLeft && srcRight >= destRight)
                        || (srcLeft >= destLeft && srcRight <= destRight))
                    {
                        overlapValue = 1.0f;
                    }
                    else
                    {
                        float x1 = max(srcLeft, destLeft);
                        float x2 = min(srcRight, destRight);
                        if (x1 <= x2)
                        {
                            float overlap = x2 - x1;
                            overlapValue = max(overlap / (srcRight - srcLeft), overlap / (destRight - destLeft));
                        }
                    }
                }
                else // Command::Left || Command::Right
                {
                    float destTop = destPoints.GetAxisAlignedTopLeft().GetY();
                    float destBottom = destPoints.GetAxisAlignedBottomLeft().GetY();
                    float srcTop = srcPoints.GetAxisAlignedTopLeft().GetY();
                    float srcBottom = srcPoints.GetAxisAlignedBottomLeft().GetY();

                    if ((srcTop <= destTop && srcBottom >= destBottom)
                        || (srcTop >= destTop && srcBottom <= destBottom))
                    {
                        overlapValue = 1.0f;
                    }
                    else
                    {
                        float y1 = max(srcTop, destTop);
                        float y2 = min(srcBottom, destBottom);
                        if (y1 <= y2)
                        {
                            float overlap = y2 - y1;
                            overlapValue = max(overlap / (srcBottom - srcTop), overlap / (destBottom - destTop));
                        }
                    }
                }

                // Set src and dest points used for distance test
                AZ::Vector2 srcPoint;
                AZ::Vector2 destPoint;
                if ((command == Command::Up) || command == Command::Down)
                {
                    float srcY;
                    float destY;
                    if (command == Command::Up)
                    {
                        srcY = srcPoints.GetAxisAlignedTopLeft().GetY();
                        destY = destPoints.GetAxisAlignedBottomLeft().GetY();
                        if (destY > srcY)
                        {
                            destY = srcY;
                        }
                    }
                    else // Command::Down
                    {
                        srcY = srcPoints.GetAxisAlignedBottomLeft().GetY();
                        destY = destPoints.GetAxisAlignedTopLeft().GetY();
                        if (destY < srcY)
                        {
                            destY = srcY;
                        }
                    }

                    srcPoint = AZ::Vector2((overlapValue < 1.0f ? srcCenter.GetX() : destCenter.GetX()), srcY);
                    destPoint = AZ::Vector2(destCenter.GetX(), destY);
                }
                else // Command::Left || Command::Right
                {
                    float srcX;
                    float destX;
                    if (command == Command::Left)
                    {
                        srcX = srcPoints.GetAxisAlignedTopLeft().GetX();
                        destX = destPoints.GetAxisAlignedTopRight().GetX();
                        if (destX > srcX)
                        {
                            destX = srcX;
                        }
                    }
                    else // Command::Right
                    {
                        srcX = srcPoints.GetAxisAlignedTopRight().GetX();
                        destX = destPoints.GetAxisAlignedTopLeft().GetX();
                        if (destX < srcX)
                        {
                            destX = srcX;
                        }
                    }

                    srcPoint = AZ::Vector2(srcX, (overlapValue < 1.0f ? srcCenter.GetY() : destCenter.GetY()));
                    destPoint = AZ::Vector2(destX, destCenter.GetY());
                }

                // Calculate angle distance value from 0 to 1
                float angleDist;
                AZ::Vector2 dir = destPoint - srcPoint;
                float angle = RAD2DEG(atan2(-dir.GetY(), dir.GetX()));
                if (angle < 0.0f)
                {
                    angle += 360.0f;
                }

                if (command == Command::Up)
                {
                    angleDist = fabs(90.0f - angle);
                }
                else if (command == Command::Down)
                {
                    angleDist = fabs(270.0f - angle);
                }
                else if (command == Command::Left)
                {
                    angleDist = fabs(180.0f - angle);
                }
                else // Command::Right
                {
                    angleDist = fabs((angle <= 180.0f ? 0.0f : 360.0f) - angle);
                }
                float angleValue = angleDist / 90.0f;

                // Calculate final distance value biased by overlap and angle values
                float dist = (destPoint - srcPoint).GetLength();
                const float distMultConstant = 1.0f;
                dist += dist * distMultConstant * angleValue * (1.0f - overlapValue);

                bool inside = true;
                if (parentElement.IsValid() && !isCurElementDescendantOfParentElement)
                {
                    // Check if the element is inside the bounds of its parent
                    UiTransformInterface::RectPoints  destPointsFromViewport = destPoints.Transform(parentTransformFromViewport);

                    AZ::Vector2 center = destPointsFromViewport.GetCenter();
                    inside = (center.GetX() >= parentRect.left &&
                              center.GetX() <= parentRect.right &&
                              center.GetY() >= parentRect.top &&
                              center.GetY() <= parentRect.bottom);
                }

                if (inside)
                {
                    if (dist < shortestDist)
                    {
                        shortestDist = dist;
                        shortestCenterToCenterDist = (destCenter - srcCenter).GetLengthSq();
                        closestElement = navigableElement->GetId();
                    }
                    else if (dist == shortestDist)
                    {
                        // Break a tie using center to center distance
                        float centerToCenterDist = (destCenter - srcCenter).GetLengthSq();
                        if (centerToCenterDist < shortestCenterToCenterDist)
                        {
                            shortestCenterToCenterDist = centerToCenterDist;
                            closestElement = navigableElement->GetId();
                        }
                    }
                }
                else
                {
                    if (dist < shortestOutsideDist)
                    {
                        shortestOutsideDist = dist;
                        shortestOutsideCenterToCenterDist = (destCenter - srcCenter).GetLengthSq();
                        closestOutsideElement = navigableElement->GetId();
                    }
                    else if (dist == shortestOutsideDist)
                    {
                        // Break a tie using center to center distance
                        float centerToCenterDist = (destCenter - srcCenter).GetLengthSq();
                        if (centerToCenterDist < shortestOutsideCenterToCenterDist)
                        {
                            shortestOutsideCenterToCenterDist = centerToCenterDist;
                            closestOutsideElement = navigableElement->GetId();
                        }
                    }
                }
            }
        }

        return closestElement.IsValid() ? closestElement : closestOutsideElement;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId FollowCustomLink(AZ::EntityId curEntityId, Command command)
    {
        AZ::EntityId nextEntityId;

        // Ask the current interactable what the next interactable should be
        if (command == Command::Up)
        {
            UiNavigationBus::EventResult(nextEntityId, curEntityId, &UiNavigationBus::Events::GetOnUpEntity);
        }
        else if (command == Command::Down)
        {
            UiNavigationBus::EventResult(nextEntityId, curEntityId, &UiNavigationBus::Events::GetOnDownEntity);
        }
        else if (command == Command::Left)
        {
            UiNavigationBus::EventResult(nextEntityId, curEntityId, &UiNavigationBus::Events::GetOnLeftEntity);
        }
        else if (command == Command::Right)
        {
            UiNavigationBus::EventResult(nextEntityId, curEntityId, &UiNavigationBus::Events::GetOnRightEntity);
        }

        return nextEntityId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsInteractableNavigable(AZ::EntityId interactableEntityId)
    {
        bool navigable = false;

        UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
        UiNavigationBus::EventResult(navigationMode, interactableEntityId, &UiNavigationBus::Events::GetNavigationMode);

        if (navigationMode != UiNavigationInterface::NavigationMode::None)
        {
            // Check if the interactable is enabled
            bool isEnabled = false;
            UiElementBus::EventResult(isEnabled, interactableEntityId, &UiElementBus::Events::IsEnabled);

            if (isEnabled)
            {
                // Check if the interactable is handling events
                UiInteractableBus::EventResult(navigable, interactableEntityId, &UiInteractableBus::Events::IsHandlingEvents);
            }
        }

        return navigable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsElementInteractableAndNavigable(AZ::EntityId entityId)
    {
        bool navigable = false;

        // Check if the element handles navigation events, we are specifically looking for interactables
        if (UiInteractableBus::FindFirstHandler(entityId))
        {
            navigable = IsInteractableNavigable(entityId);
        }

        return navigable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void FindNavigableInteractables(AZ::EntityId parentElement, AZ::EntityId ignoreElement, LyShine::EntityArray& result)
    {
        LyShine::EntityArray elements;
        UiElementBus::EventResult(elements, parentElement, &UiElementBus::Events::GetChildElements);

        AZStd::list<AZ::Entity*> elementList(elements.begin(), elements.end());
        while (!elementList.empty())
        {
            auto& entity = elementList.front();

            // Check if the element handles navigation events, we are specifically looking for interactables
            bool handlesNavigationEvents = false;
            if (UiInteractableBus::FindFirstHandler(entity->GetId()))
            {
                UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
                UiNavigationBus::EventResult(navigationMode, entity->GetId(), &UiNavigationBus::Events::GetNavigationMode);
                handlesNavigationEvents = (navigationMode != UiNavigationInterface::NavigationMode::None);
            }

            // Check if the element is enabled
            bool isEnabled = false;
            UiElementBus::EventResult(isEnabled, entity->GetId(), &UiElementBus::Events::IsEnabled);

            bool navigable = false;
            if (handlesNavigationEvents && isEnabled && (!ignoreElement.IsValid() || entity->GetId() != ignoreElement))
            {
                // Check if the element is handling events
                bool isHandlingEvents = false;
                UiInteractableBus::EventResult(isHandlingEvents, entity->GetId(), &UiInteractableBus::Events::IsHandlingEvents);
                navigable = isHandlingEvents;
            }

            if (navigable)
            {
                result.push_back(entity);
            }

            if (!handlesNavigationEvents && isEnabled)
            {
                LyShine::EntityArray childElements;
                UiElementBus::EventResult(childElements, entity->GetId(), &UiElementBus::Events::GetChildElements);
                elementList.insert(elementList.end(), childElements.begin(), childElements.end());
            }

            elementList.pop_front();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId FindAncestorNavigableInteractable(AZ::EntityId childInteractable, bool ignoreAutoActivatedAncestors)
    {
        AZ::EntityId parent;
        UiElementBus::EventResult(parent, childInteractable, &UiElementBus::Events::GetParentEntityId);
        while (parent.IsValid())
        {
            if (UiNavigationHelpers::IsElementInteractableAndNavigable(parent))
            {
                if (ignoreAutoActivatedAncestors)
                {
                    // Check if this hover interactable should automatically go to an active state
                    bool autoActivated = false;
                    UiInteractableBus::EventResult(autoActivated, parent, &UiInteractableBus::Events::GetIsAutoActivationEnabled);
                    if (!autoActivated)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            AZ::EntityId newParent = parent;
            parent.SetInvalid();
            UiElementBus::EventResult(parent, newParent, &UiElementBus::Events::GetParentEntityId);
        }

        return parent;
    }
} // namespace UiNavigationHelpers
