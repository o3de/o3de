# Entity Presets

Ready-made entities, offered under **Create Preset** in the viewport and Entity Outliner
right-click menus. One click instead of create-entity, then hunt for the components, then set the
two properties that make it useful.

Presets are **data**. Almost every one ships inside the gem that owns the components it names, so a
preset appears exactly when the thing it builds is available, and disappears with its gem.

## Shipping presets from a gem

Put one or more `*.entitypresets.json` files in a `Presets` folder at the root of the gem, beside
`gem.json`:

    Gems/LmbrCentral/Presets/shapes.entitypresets.json
    Gems/LmbrCentral/Presets/audio.entitypresets.json

Every enabled gem is scanned at editor start. No registration, no code, no CMake entry - the folder
is copied into the install layout automatically.

The `.entitypresets.json` suffix is required, not decorative: it is what lets a gem keep unrelated
JSON in the same folder without this feature claiming it.

### File format

The file is a standard O3DE serialized object, so it needs the `JsonSerialization` header. A file
without it parses as JSON and is then refused, and the presets simply never appear - so if a gem's
presets are missing, check this first.

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PresetFile",
    "ClassData": {
        "presets": [
            {
                "name": "Spot Light (Disk)",
                "description": "Optional. Shown as the menu tooltip - say what it makes, and anything the user still has to do.",
                "category": "Lights",
                "components": [
                    {
                        "component": "Light",
                        "properties": [
                            {
                                "path": "Controller|Configuration|Light type",
                                "type": "int",
                                "value": 2
                            }
                        ]
                    }
                ]
            }
        ]
    }
}
```

The editor writes this exact shape when saving user presets, so the quickest way to author a gem
file is to build the presets in **Manage Presets...**, then copy them out of the project's
`Registry/EntityPresets.json`.

| Field | Meaning |
| --- | --- |
| `name` | Menu entry, and the new entity's name. |
| `description` | Optional. Becomes the action's tooltip. |
| `category` | Groups presets into a submenu. Categories with the same name merge, across gems. |
| `components` | Added to the new entity, in order. |
| `levelComponents` | Optional. Ensured on the **level** entity first, and only if absent. |

`type` is one of `bool`, `int`, `double`, `string`, `asset`. It is carried explicitly because JSON
cannot say whether `4` meant an integer or a float, nor whether a string meant text or an asset
path. Use `int` for enums.

### Getting the names right

`component` is the **display name**, exactly as it appears in Add Component - `"PhysX Primitive
Collider"`, not the C++ class. A name that does not resolve is reported in the console.

`path` is the Edit Context path, with `|` between levels. If you get one wrong the console prints
every valid path for that component, which is the fastest way to find the right one.

`value` for an `asset` is the relative product path in the cache, such as
`objects/_primitives/_box_1x1.fbx.azmodel`. An unresolved asset leaves the property at its default
rather than clearing it.

### Two things that fail quietly

**Required services.** A component whose requirements are unmet is added *pending*: the entity
looks right, the component sits greyed out, and creation reports success. A `PhysX Primitive
Collider` needs a rigid body; a `Reflection Probe` needs a box shape; an `Audio Trigger` needs an
Audio Proxy. Creation warns when this happens - list the prerequisite in `components` and put it
first.

**Cross-gem components.** Naming a component from another gem is fine when your gem depends on it,
and a trap when it does not. Prefer to ship a preset from the gem that owns its distinctive part,
and check `gem.json` before reaching across.

## Presets that need code

Some setups cannot be expressed as data: they touch the level entity, build a hierarchy, or wire
entities to each other. `levelComponents` covers the first. For the rest - the Terrain preset is
the only one in the engine - a gem registers its own action and hangs it off the Create Preset menu.

`EntityPresetsIdentifiers.h` publishes what that needs:

- `EntityPresetsRootMenuIdentifier` - the menu to attach to.
- `EntityPresetsGemSortKeyStart` - the sort-key band reserved for gems, after the categories and
  before Manage Presets. Category count varies with what is loaded, so a key cannot be computed
  from outside; use the band.

No handler ordering is needed. `ActionManagerSystemComponent` broadcasts each registration hook to
every handler before moving to the next, so the root menu exists by the time any handler receives
`OnMenuBindingHook`. Register your menu and actions in the registration hooks; only call
`AddSubMenuToMenu` in the binding hook. `Gems/Terrain/Code/Source/EntityPresets` is a worked example.

`PresetRequirements.h` offers the matching preflight, so a code preset can check what it needs and
say so before creating anything - `MissingComponents` to test availability, and two dialogs for the
"cannot build" and "build a reduced version?" cases. It lives in AzToolsFramework so a gem does not
need Qt to use it.

## User presets

Anything the user adds through **Manage Presets...** is written to `Registry/EntityPresets.json`
inside the project, so their presets travel with the project and can be committed and shared.
Gem-supplied presets are read-only in that dialog - editing one would be overwritten the next time
the gem updated - and duplicating gives an editable copy.
