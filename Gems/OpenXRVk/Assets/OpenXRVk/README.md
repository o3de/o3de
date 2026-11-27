# About system.xrprofiles and default.xractions

Both of these assets are editable via the `Asset Editor` UI. The `Asset Editor` is a tool provided by the O3DE Editor.

`system.xrprofiles` defines a list of standard OpenXR interaction profiles that have been tested with O3DE.
For more information read the header file: `.../Gems/OpenXRVk/Code/Include/OpenXRVk/OpenXRVkInteractionProfilesAsset.h`.

`default.xraction` depends on `system.xrprofiles` and it defines a set of actions that your application can use
to read user input or drive haptic feedback signals. This is the default asset that the OpenXRVk Gem will load, but can be overriden with help of the following Registry Key:
**"/O3DE/Atom/OpenXR/ActionSetsAsset"**. If this key is not defined, the application will default to: **"openxrvk/default.xractions"**.  
  
Here is an example of an application named `AdventureVR` that customizes the Action Sets asset:
- ActionSet Asset Location: \<AdventureVR\>/Assets/AdventureVR/adventurevr.xractions
- Registry File Location: \<AdventureVR\>/Registry/adventurevr.setreg, with the following content:  
```json
{
    "O3DE": {
        "Atom": {
            "OpenXR": {
                "ActionSetsAsset": "adventurevr/adventurevr.xractions"
            }
        }
    }
}
```
