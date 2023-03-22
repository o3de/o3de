import azlmbr.math as math
import azlmbr.editor as editor
import azlmbr.bus as bus
import azlmbr.legacy.general as general
from azpy.dcc.o3de.utils.atom_constants import AtomComponentProperties


# Light type options for the Light component.
LIGHT_TYPES = {
    'unknown': 0,
    'sphere': 1,
    'spot_disk': 2,
    'capsule': 3,
    'quad': 4,
    'polygon': 5,
    'simple_point': 6,
    'simple_spot': 7,
}


# Component paths for Directional Light settings
LIGHT_COMPONENT_PROPERTY_PATHS = [
    'Controller',
    'Controller|Configuration',
    'Controller|Configuration|Color',
    'Controller|Configuration|Intensity Mode',
    'Controller|Configuration|Intensity',
    'Controller|Configuration|Angular Diameter',
    'Controller|Configuration|Shadow|Camera',
    'Controller|Configuration|Shadow|Shadow Far Clip',
    'Controller|Configuration|Shadow|Shadowmap Size',
    'Controller|Configuration|Shadow|Cascade Count',
    'Controller|Configuration|Shadow|Split Automatic',
    'Controller|Configuration|Shadow|Split Ratio',
    'Controller|Configuration|Shadow|Far Depth Cascade',
    'Controller|Configuration|Shadow|Ground Height',
    'Controller|Configuration|Shadow|Enable Cascade Correction?',
    'Controller|Configuration|Shadow|Enable Debug Coloring?',
    'Controller|Configuration|Shadow|Shadow Filter Method',
    'Controller|Configuration|Shadow|Softening Boundary Width',
    'Controller|Configuration|Shadow|Prediction Sample Count',
    'Controller|Configuration|Shadow|Filtering Sample Count'
]


def get_component_path(str_match):
    for path in LIGHT_COMPONENT_PROPERTY_PATHS:
        if path.endswith(str_match):
            return path


def set_light_type(light_component, light_type):
    light_component.set_component_property_value(AtomComponentProperties.light('Light type'), LIGHT_TYPES[light_type])
    general.idle_wait_frames(1)


def set_component_values(component, component_data):
    if component == 'Directional Light':
        set_directional_light_values(component_data)
    else:
        light_type = component_data['attributes']['component_class']
        set_light_type(component_data['component'], light_type)
        if light_type == 'simple_spot':
            set_spot_light_values(component_data)
        elif light_type == 'simple_point':
            set_point_light_values(component_data)
        elif light_type == 'quad':
            set_area_light_values(component_data)


def set_directional_light_values(component_data):
    print(f'Set Directional Light Values: {component_data}')
    for attribute_name, attribute_value in component_data['attributes'].items():
        target = component_data['component']
        if attribute_name == 'color':
            component_path = get_component_path('|Color')
            set_component_color_value(target, component_path, attribute_value)
        elif attribute_name == 'intensity':
            component_path = get_component_path('|Intensity')
            set_component_float_value(target, component_path, attribute_value)
        general.idle_wait_frames(1)


def set_point_light_values(component_data):
    for attribute_name, attribute_value in component_data['attributes'].items():
        target = component_data['component']
        if attribute_name == 'color':
            component_path = get_component_path('|Color')
            set_component_color_value(target, component_path, attribute_value)
        elif attribute_name == 'intensity':
            component_path = get_component_path('|Intensity')
            set_component_float_value(target, component_path, attribute_value)
        general.idle_wait_frames(1)


def set_area_light_values(component_data):
    for attribute_name, attribute_value in component_data['attributes'].items():
        target = component_data['component']


def set_spot_light_values(component_data):
    print(f'Setting spotlight values: {component_data}')
    for attribute_name, attribute_value in component_data['attributes'].items():
        target = component_data['component']
        if attribute_name == 'coneAngle':
            component_path = 'Controller|Configuration|Shutters|Inner angle'
            set_component_float_value(target, component_path, attribute_value)
        elif attribute_name == 'penumbraAngle':
            component_path = 'Controller|Configuration|Shutters|Outer angle'
            cone_angle = component_data['attributes']['coneAngle']
            penumbra_angle = component_data['attributes']['penumbraAngle']
            attribute_value = cone_angle + penumbra_angle
            set_component_float_value(target, component_path, attribute_value)
        elif attribute_name == 'color':
            component_path = get_component_path('|Color')
            set_component_color_value(target, component_path, attribute_value)
        elif attribute_name == 'intensity':
            component_path = get_component_path('|Intensity')
            set_component_float_value(target, component_path, attribute_value)
        general.idle_wait_frames(1)


def set_component_color_value(target_component, component_path, color):
    color_rgba = math.Color(color[0], color[1], color[2], 1.0)
    target_component.set_component_property_value(component_path, color_rgba)


def set_component_float_value(target_component, component_path, float_value):
    target_component.set_component_property_value(component_path, float_value)
