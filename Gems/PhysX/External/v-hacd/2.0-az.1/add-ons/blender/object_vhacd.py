# ----------------
# V-HACD Blender add-on
# Copyright (c) 2014, Alain Ducharme
# ----------------
# This software is provided 'as-is', without any express or implied warranty.
# In no event will the authors be held liable for any damages arising from the use of this software.
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it freely,
# subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

#
# NOTE: requires/calls Khaled Mamou's testVHACD executable found here: http://code.google.com/p/v-hacd/
#

bl_info = {
    'name': 'V-HACD',
    'description': 'Hierarchical Approximate Convex Decomposition',
    'author': 'Alain Ducharme (Phymec)',
    'version': (0, 2),
    'blender': (2, 65, 0),
    'location': '3D View, Tools tab -> V-HACD, in Object mode',
    'warning': "Requires Khaled Mamou's V-HACD v2.0 textVHACD executable: (see documentation)",
    'wiki_url': 'http://code.google.com/p/v-hacd/',
    'category': 'Object',
}

import bpy
from bpy.props import BoolProperty, EnumProperty, FloatProperty, IntProperty, PointerProperty, StringProperty
from bl_operators.presets import AddPresetBase
import bmesh
from mathutils import Matrix, Vector
from os import path as os_path
from subprocess import Popen
from tempfile import gettempdir

def physics_mass_center(mesh):
    '''Calculate (final triangulated) mesh's mass and center of mass based on volume'''
    volume = 0.
    mass = 0.
    com = Vector()
    # Based on Stan Melax's volint
    for face in mesh.polygons:
        a = Matrix((mesh.vertices[face.vertices[0]].co, mesh.vertices[face.vertices[1]].co, mesh.vertices[face.vertices[2]].co))
        vol = a.determinant()
        volume += vol
        com += vol * (a[0] + a[1] + a[2])
    if volume > 0:
        com /= volume * 4.
        mass = volume / 6.
        return mass, com
    else:
        return mass, Vector()

def off_export(mesh, fullpath):
    '''Export triangulated mesh to Object File Format'''
    with open(fullpath, 'wb') as off:
        off.write(b'OFF\n')
        off.write(str.encode('{} {} 0\n'.format(len(mesh.vertices), len(mesh.polygons))))
        for vert in mesh.vertices:
            off.write(str.encode('{:g} {:g} {:g}\n'.format(*vert.co)))
        for face in mesh.polygons:
            off.write(str.encode('3 {} {} {}\n'.format(*face.vertices)))

class VHACD(bpy.types.Operator):
    bl_idname = 'object.vhacd'
    bl_label = 'Hierarchical Approximate Convex Decomposition'
    bl_description = 'Hierarchical Approximate Convex Decomposition, see http://code.google.com/p/v-hacd/'
    bl_options = {'PRESET'}

    # -------------------
    # pre-process options
    remove_doubles = BoolProperty(
            name = 'Remove Doubles',
            description = 'Collapse overlapping vertices in generated mesh',
            default = True)

    apply_transforms = EnumProperty(
            name = 'Apply',
            description = 'Apply Transformations to generated mesh',
            items = (
                ('LRS', 'Location + Rotation + Scale', 'Apply location, rotation and scale'),
                ('RS', 'Rotation + Scale', 'Apply rotation and scale'),
                ('S', 'Scale', 'Apply scale only'),
                ('NONE', 'None', 'Do not apply transformations'),
                ),
            default = 'RS')

    # ---------------
    # VHACD parameters
    resolution = IntProperty(
            name = 'Voxel Resolution',
            description = 'Maximum number of voxels generated during the voxelization stage',
            default = 100000, min = 10000, max = 64000000)

    depth = IntProperty(
            name = 'Clipping Depth',
            description = 'Maximum number of clipping stages. During each split stage, all the model parts (with a concavity higher than the user defined threshold) are clipped according the "best" clipping plane',
            default = 20, min = 1, max = 32)

    concavity = FloatProperty(
            name = 'Maximum Concavity',
            description = 'Maximum concavity',
            default = 0.0025, min = 0.0, max = 1.0, precision = 4)

    planeDownsampling = IntProperty(
            name = 'Plane Downsampling',
            description = 'Granularity of the search for the "best" clipping plane',
            default = 4, min = 1, max = 16)

    convexhullDownsampling = IntProperty(
            name = 'Convex Hull Downsampling',
            description = 'Precision of the convex-hull generation process during the clipping plane selection stage',
            default = 4, min = 1, max = 16)

    alpha = FloatProperty(
            name = 'Alpha',
            description = 'Bias toward clipping along symmetry planes',
            default = 0.05, min = 0.0, max = 1.0, precision = 4)

    beta = FloatProperty(
            name = 'Beta',
            description = 'Bias toward clipping along revolution axes',
            default = 0.05, min = 0.0, max = 1.0, precision = 4)

    gamma = FloatProperty(
            name = 'Gamma',
            description = 'Maximum allowed concavity during the merge stage',
            default = 0.00125, min = 0.0, max = 1.0, precision = 5)

    pca = BoolProperty(
            name = 'PCA',
            description = 'Enable/disable normalizing the mesh before applying the convex decomposition',
            default = False)

    mode = EnumProperty(
            name = 'ACD Mode',
            description = 'Approximate convex decomposition mode',
            items = (
                ('VOXEL', 'Voxel', 'Voxel ACD Mode'),
                ('TETRAHEDRON', 'Tetrahedron', 'Tetrahedron ACD Mode')),
            default = 'VOXEL')

    maxNumVerticesPerCH = IntProperty(
            name = 'Maximum Vertices Per CH',
            description = 'Maximum number of vertices per convex-hull',
            default = 32, min = 4, max = 1024)

    minVolumePerCH = FloatProperty(
            name = 'Minimum Volume Per CH',
            description = 'Minimum volume to add vertices to convex-hulls',
            default = 0.0001, min = 0.0, max = 0.01, precision = 5)

    # -------------------
    # post-process options
    show_transparent = BoolProperty(
            name = 'Show Transparent',
            description = 'Enable transparency for ACD hulls',
            default=True)

    use_generated = BoolProperty(
            name = 'Use Generated Mesh',
            description = 'Use triangulated mesh generated for V-HACD (for game engine visuals; otherwise use original object)',
            default=True)

    hide_render = BoolProperty(
            name = 'Hide Render',
            description = 'Disable rendering of convex hulls (for game engine)',
            default=True)

    mass_com = BoolProperty(
            name = 'Center of Mass',
            description = 'Calculate physics mass and set center of mass (origin) based on volume and density (best to apply rotation and scale)',
            default=True)

    density = FloatProperty(
            name = 'Density',
            description = 'Material density used to calculate mass from volume',
            default = 10.0, min = 0.0)

    def execute(self, context):

        # Check textVHACD executable path
        vhacd_path = bpy.path.abspath(context.scene.vhacd.vhacd_path)
        if os_path.isdir(vhacd_path):
            vhacd_path = os_path.join(vhacd_path, 'testVHACD')
        elif not os_path.isfile(vhacd_path):
            self.report({'ERROR'}, 'Path to testVHACD executable required')
            return {'CANCELLED'}
        if not os_path.exists(vhacd_path):
            self.report({'ERROR'}, 'Cannot find testVHACD executable at specified path')
            return {'CANCELLED'}

        # Check Data path
        data_path = bpy.path.abspath(context.scene.vhacd.data_path)
        if data_path.endswith('/') or data_path.endswith('\\'):
            data_path = os_path.dirname(data_path)
        if not os_path.exists(data_path):
            self.report({'ERROR'}, 'Invalid data directory')
            return {'CANCELLED'}

        selected = bpy.context.selected_objects
        if not selected:
            self.report({'ERROR'}, 'Object(s) must be selected first')
            return {'CANCELLED'}
        for ob in selected:
            ob.select = False

        new_objects = []
        for ob in selected:
            filename = ''.join(c for c in ob.name if c.isalnum() or c in (' ','.','_')).rstrip()

            off_filename = os_path.join(data_path, '{}.off'.format(filename))
            outFileName = os_path.join(data_path, '{}.wrl'.format(filename))
            logFileName = os_path.join(data_path, '{}_log.txt'.format(filename))

            try:
                mesh = ob.to_mesh(context.scene, True, 'PREVIEW', calc_tessface=False)
            except:
                continue

            translation, quaternion, scale = ob.matrix_world.decompose()
            scale_matrix = Matrix(((scale.x,0,0,0),(0,scale.y,0,0),(0,0,scale.z,0),(0,0,0,1)))
            if self.apply_transforms in ['S', 'RS', 'LRS']:
                pre_matrix = scale_matrix
                post_matrix = Matrix()
            else:
                pre_matrix = Matrix()
                post_matrix = scale_matrix
            if self.apply_transforms in ['RS', 'LRS']:
                pre_matrix = quaternion.to_matrix().to_4x4() * pre_matrix
            else:
                post_matrix = quaternion.to_matrix().to_4x4() * post_matrix
            if self.apply_transforms == 'LRS':
                pre_matrix = Matrix.Translation(translation) * pre_matrix
            else:
                post_matrix = Matrix.Translation(translation) * post_matrix

            mesh.transform(pre_matrix)

            bm = bmesh.new()
            bm.from_mesh(mesh)
            if self.remove_doubles:
                bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0001)
            bmesh.ops.triangulate(bm, faces=bm.faces)
            bm.to_mesh(mesh)
            bm.free()

            print('\nExporting mesh for V-HACD: {}...'.format(off_filename))
            off_export(mesh, off_filename)
            cmd_line = '"{}" --input "{}" --resolution {} --depth {} --concavity {:g} --planeDownsampling {} --convexhullDownsampling {} --alpha {:g} --beta {:g} --gamma {:g} --pca {:b} --mode {:b} --maxNumVerticesPerCH {} --minVolumePerCH {:g} --output "{}" --log "{}"'.format(
                    vhacd_path,
                    off_filename,
                    self.resolution,
                    self.depth,
                    self.concavity,
                    self.planeDownsampling,
                    self.convexhullDownsampling,
                    self.alpha,
                    self.beta,
                    self.gamma,
                    self.pca,
                    self.mode == 'TETRAHEDRON',
                    self.maxNumVerticesPerCH,
                    self.minVolumePerCH,
                    outFileName,
                    logFileName)

            print('Running V-HACD...\n{}\n'.format(cmd_line))
            vhacd_process = Popen(cmd_line, bufsize=-1, close_fds=True, shell=True)

            mass = 1.0
            com = Vector()
            if self.mass_com:
                mass, com = physics_mass_center(mesh)
                mass *= self.density
                post_matrix = Matrix.Translation(com * post_matrix) * post_matrix
                pre_matrix = Matrix.Translation(-com) * pre_matrix
            if not self.use_generated:
                bpy.data.meshes.remove(mesh)

            vhacd_process.wait()
            if not os_path.exists(outFileName):
                continue

            bpy.ops.import_scene.x3d(filepath=outFileName, axis_forward='Y', axis_up='Z')
            imported = bpy.context.selected_objects
            new_objects.extend(imported)
            parent = None

            for hull in imported:
                # Make hull a compound rigid body
                hull.select = False
                hull.show_transparent = self.show_transparent
                if self.mass_com:
                    for vert in hull.data.vertices:
                        vert.co -= com
                hull.game.physics_type = 'RIGID_BODY'
                hull.game.use_collision_bounds = True
                hull.game.collision_bounds_type = 'CONVEX_HULL'
                hull.game.mass = mass
                hull.game.use_collision_compound = True
                hull.hide_render = self.hide_render
                if not parent:
                    # Use first hull as compound parent
                    parent = hull
                    hull.matrix_basis = post_matrix
                    # Attach visual mesh as child...
                    ob.game.physics_type = 'NO_COLLISION'
                    if self.use_generated:
                        if self.mass_com:
                            for vert in mesh.vertices:
                                vert.co -= com
                        obc = bpy.data.objects.new(ob.name + '_GM', mesh)
                        bpy.context.scene.objects.link(obc)
                        obc.game.physics_type = 'NO_COLLISION'
                        obc.parent = parent
                        new_objects.append(obc)
                    else:
                        ob.matrix_basis = pre_matrix
                        ob.parent = parent
                else:
                    hull.parent = parent
                new_name = ob.name + '_ACD'
                hull.name = hull.name.replace('ShapeIndexedFaceSet', new_name)
                hull.data.name = hull.data.name.replace('ShapeIndexedFaceSet', new_name)

        if len(new_objects):
            for ob in new_objects:
                ob.select = True
        else:
            for ob in selected:
                ob.select = True
            self.report({'WARNING'}, 'No meshes to process!')
            return {'CANCELLED'}

        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self, width=384)

    def draw(self, context):
        layout = self.layout
        col = layout.column()
        col.label('Pre-Processing Options (generated mesh):')
        row = col.row()
        row.prop(self, 'remove_doubles')
        row.prop(self, 'apply_transforms')

        layout.separator()
        col = layout.column()
        col.label('V-HACD Parameters:')
        col.prop(self, 'resolution')
        col.prop(self, 'depth')
        col.prop(self, 'concavity')
        col.prop(self, 'planeDownsampling')
        col.prop(self, 'convexhullDownsampling')
        row = col.row()
        row.prop(self, 'alpha')
        row.prop(self, 'beta')
        row.prop(self, 'gamma')
        row = col.row()
        row.prop(self, 'pca')
        row.prop(self, 'mode')
        col.prop(self, 'maxNumVerticesPerCH')
        col.prop(self, 'minVolumePerCH')

        layout.separator()
        col = layout.column()
        col.label('Post-Processing Options:')
        row = col.row()
        row.prop(self, 'show_transparent')
        row.prop(self, 'use_generated')
        row = col.row()
        row.prop(self, 'hide_render')
        row.prop(self, 'mass_com')
        row.prop(self, 'density')

        layout.separator()
        col = layout.column()
        col.label('WARNING:', icon='ERROR')
        col.label(' -> Processing can take several minutes per object!')
        col.label(' -> ALL selected objects will be processed sequentially!')
        col.label(' -> Game Engine physics compound generated for each object')
        col.label(' -> See Console Window for progress..,')

class VIEW3D_PT_tools_vhacd(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'TOOLS'
    bl_category = 'Tools'
    bl_label = 'V-HACD'
    bl_context = 'objectmode'
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        row = layout.row(align=True)
        row.menu('VHACD_MT_path_presets', text=bpy.types.VHACD_MT_path_presets.bl_label)
        row.operator('scene.vhacd_preset_add', text='', icon='ZOOMIN')
        row.operator('scene.vhacd_preset_add', text='', icon='ZOOMOUT').remove_active = True
        col = layout.column()
        col.prop(context.scene.vhacd, 'vhacd_path')
        col.prop(context.scene.vhacd, 'data_path')
        col.operator('object.vhacd', text='V-HACD')

class AddPresetVHACD(AddPresetBase, bpy.types.Operator):
    '''Add V-HACD Paths Preset'''
    bl_idname = 'scene.vhacd_preset_add'
    bl_label = 'Add VHACD Path Preset'
    preset_menu = 'VHACD_MT_path_presets'

    preset_defines = ['vhacd = bpy.context.scene.vhacd']

    preset_values = [
        'vhacd.vhacd_path',
        'vhacd.data_path',
    ]
    preset_subdir = 'vhacd'

class VHACD_MT_path_presets(bpy.types.Menu):
    bl_label = 'V-HACD Path Presets'
    preset_subdir = 'vhacd'
    preset_operator = 'script.execute_preset'
    draw = bpy.types.Menu.draw_preset

class VHACDPaths(bpy.types.PropertyGroup):
    @classmethod
    def register(cls):
        bpy.types.Scene.vhacd = PointerProperty(
                name = 'V-HACD Settings',
                description = 'V-HACD settings',
                type = cls,
                )
        cls.vhacd_path = StringProperty(
                name = 'VHACD Path',
                description = 'Path to testVHACD executable',
                default = '', maxlen = 1024, subtype = 'FILE_PATH')

        cls.data_path = StringProperty(
                name = 'Data Path',
                description = 'Data path to store V-HACD meshes and logs',
                default = gettempdir(), maxlen = 1024, subtype = 'DIR_PATH')

    @classmethod
    def unregister(cls):
        if 'vhacd' in dir(bpy.types.Scene):
            del bpy.types.Scene.vhacd

def register():
    bpy.utils.register_module(__name__)

def unregister():
    bpy.utils.unregister_module(__name__)
