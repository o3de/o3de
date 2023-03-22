import azlmbr.legacy.general as general


def set_component_values(component, component_data):
    print(f'Setting component value in O3DE Cameras [{component}] ::: {component_data}')

    for key, value in component_data.items():
        if key == 'focal_length':
            pass
        elif key == 'near_clip_distance':
            component_path = 'Controller|Configuration|Near clip distance'
            set_component_float_value(component, component_path, value)
        elif key == 'far_clip_distance':
            component_path = 'Controller|Configuration|Far clip distance'
            set_component_float_value(component, component_path, value)
        general.idle_wait_frames(1)


def set_component_float_value(target_component, component_path, float_value):
    target_component.set_component_property_value(component_path, float_value)