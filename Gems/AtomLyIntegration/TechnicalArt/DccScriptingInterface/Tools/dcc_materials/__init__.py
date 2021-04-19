# from maya_mapping import MayaMapping
# from max_mapping import MaxMapping
# from blender_mapping import BlenderMapping

# def get_dcc_mapping(app):
# 	app = app.lower()
# 	if app == 'maya':
# 		return MayaMapping()
# 	elif app == '3dsmax':
# 		return MaxMapping()
# 	elif app == 'blender':
# 		return BlenderMapping()
# 	else:
# 		return NullMapping(app)

__all__ = ['blender_materials',
           'dcc_material_mapping',
           'drag_and_drop',
           'main',
           'materials_export',
           'max_materials',
           'maya_materials',
           'model']