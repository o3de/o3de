# Controller Models and Animations Setup in O3DE

There are many VR devices on the market, each with different controllers, so developers are responsible for integrating the appropriate 3D models into their applications.

This Gem provides asset templates that can help you implement your own 3D controller models more easily.

The prefab at  
`OpenXRVk\Assets\Devices\Generic\Prefabs\Controllers.prefab`  
includes both left and right controllers:  
- `OpenXRVk\Assets\Devices\Generic\Prefabs\LeftController.prefab`  
- `OpenXRVk\Assets\Devices\Generic\Prefabs\RightController.prefab`

We recommend copying the entire folder  
`OpenXRVk\Assets\Devices\Generic\`  
into your own project. Then, place your controller `.fbx` models (which contain `.motion` data and animations) into the appropriate subfolders.

For example, Oculus Quest models can be downloaded from the official site:  
https://developers.meta.com/horizon/downloads/package/oculus-controller-art/

You will need to:
- Convert the `.fbx` files from ASCII to binary format (e.g., using Autodesk FBX Converter).
- Use Blender to split out individual animations for each button and joystick on the controller.

Animations can be as short as one frame (e.g., to represent a "pressed" state).  
For thumbsticks, you’ll need 4 separate animations: forward, backward, left, and right.

Here is an example animation list for Oculus Quest Pro/3:

```
left_b_button_menu.fbx  
left_b_button_x.fbx  
left_b_button_y.fbx  
left_b_thumbstick_000.fbx  
left_b_thumbstick_090.fbx  
left_b_thumbstick_180.fbx  
left_b_thumbstick_270.fbx  
left_b_trigger_front.fbx  
left_b_trigger_grip.fbx  
right_b_button_a.fbx  
right_b_button_b.fbx  
right_b_button_oculus.fbx  
right_b_thumbstick_000.fbx  
right_b_thumbstick_090.fbx  
right_b_thumbstick_180.fbx  
right_b_thumbstick_270.fbx  
right_b_trigger_front.fbx  
right_b_trigger_grip.fbx  
```

After loading the controller prefabs into the O3DE Editor, you may see missing or invalid property references.  
You will need to assign the appropriate `Actor Asset`, `Material`, and `AnimGraph`.

If you add new actions to `OpenXRVk\Assets\OpenXRVk\default.xractions` or create your own `.xractions` file, you must update the `XRControllerAnimation` component to match the correct action paths to the corresponding animations.

---

## Licensing and Distribution Notice

Due to licensing restrictions, official controller 3D models (such as those for Oculus/Meta devices) are **not** included in this repository.  
You must download them yourself from official sources and ensure your usage complies with their respective license agreements.

Only template assets and instructions for integration are provided here.  
Do **not** redistribute downloaded or proprietary models with your project unless explicitly permitted by the original license.