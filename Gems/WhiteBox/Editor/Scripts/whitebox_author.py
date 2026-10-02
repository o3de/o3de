"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Author White Box meshes from code: the values on the White Box pane plus id-based modeling tools.

    import whitebox_author as wb

    pillar = wb.WhiteBox.create("Pillar", position=(0, 0, 0), shape="cylinder", width=1, depth=1, height=3, sides=16)
    top = pillar.facing((0, 0, 1))                  # polygon ids whose normal points up
    pillar.inset(top, 0.2)                          # then pull a cap out of the top
    pillar.extrude(pillar.facing((0, 0, 1)), 0.4)

Everything is addressed by integer ids that stay valid until the next topology edit, so re-query (polygons(), edges(),
vertices()) after each call that changes the mesh. Methods raise WhiteBoxError when the Editor refuses an edit.
Enum-like arguments take a name ("cylinder", "subtract", "manifold") or its number.

Coordinates are in the active layer's own space: entity-local while the layer transform is the identity.
For an MCP server, run(entity, ops) executes a list of {"op": "<method name>", ...arguments} dicts in one undo step.
"""

import contextlib
import inspect
import json
import re

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as amath
import azlmbr.components as components

try:
    import azlmbr.whitebox.authoring as _wb
except ImportError as _exc:  # the gem was built before the authoring bus existed
    raise RuntimeError(
        "azlmbr.whitebox.authoring is missing: rebuild the WhiteBox gem and the Editor. (%s)" % _exc)

_BUS = _wb.EditorWhiteBoxAuthoringRequestBus


class WhiteBoxError(RuntimeError):
    """The Editor refused a White Box edit; the message says why."""


# ---------------------------------------------------------------- enum names

# Names for the int parameters; the values mirror the C++ enums (the Editor does not export enum constants to Python).
SHAPES = {n: i for i, n in enumerate([
    "box", "cylinder", "pyramid", "cone", "sphere", "staircase", "room", "door", "circular_stairs", "plane", "torus",
    "pipe", "polygon"])}  # DrawShapeType
COMBINE_MODES = {"separate": 0, "union": 1, "subtract": 2, "intersect": 3}  # LayerCombineMode
BOOLEAN_OPERATIONS = {"union": 0, "subtraction": 1, "intersection": 2}  # Api::BooleanOperation
SOLVERS = {"manifold": 0, "fast": 1}  # Api::CsgSolver
AFTER_APPLY = {"keep": 0, "hide": 1, "delete": 2}  # SourceAfterApply
SMOOTH_EDITS = {"set": 0, "add": 1, "remove": 2}  # Api::SmoothingEdit
UV_MODES = {"world": 0, "planar": 1, "manual": 2}  # Api::UvProjectionMode

# Struct fields that hold an enum, so a dict value may be given by name.
_ENUM_FIELDS = {
    "Shape": SHAPES,
    "CombineMode": COMBINE_MODES,
    "Operation": BOOLEAN_OPERATIONS,
    "CutterOperation": BOOLEAN_OPERATIONS,
    "CsgSolver": SOLVERS,
    "SourceAfterApply": AFTER_APPLY,
}


def _enum(table, value, what):
    if isinstance(value, bool):
        raise ValueError("%s must be a name or a number, not a bool" % what)
    if isinstance(value, int):
        return value
    key = str(value).strip().lower().replace("-", "_").replace(" ", "_")
    if key not in table:
        raise ValueError("unknown %s '%s'; choose from %s" % (what, value, sorted(table)))
    return table[key]


# ---------------------------------------------------------------- value conversion

def _vec(value):
    if getattr(value, "typename", None) == "Vector3":
        return value
    return amath.Vector3(float(value[0]), float(value[1]), float(value[2]))


def _tup(vector):
    return (float(vector.x), float(vector.y), float(vector.z))


def _color(value):
    if not isinstance(value, (list, tuple)):
        return value
    rgba = [float(x) for x in value] + [1.0] * (4 - len(value))
    return amath.Color(*rgba[:4])


def _typename(value):
    return getattr(value, "typename", None)


def _plain(value):
    """A property value as plain Python (tuples, lists, strings) so it can go through JSON."""
    name = _typename(value)
    if name == "Vector3":
        return _tup(value)
    if name == "Color":
        return (float(value.r), float(value.g), float(value.b), float(value.a))
    if name == "EntityId":
        return value.ToString() if value.IsValid() else None
    if isinstance(value, (list, tuple)):
        return [_plain(v) for v in value]
    return value


def _norm(name):
    return name.replace("_", "").lower()


# Property names of each scripted struct. The Editor does not list them through dir(), so they are spelled out here;
# they mirror the structs in EditorWhiteBoxAuthoringBus.h.
_FIELDS = {
    "WhiteBoxResult": ["Success", "Message", "Ids"],
    "WhiteBoxShapeParams": ["Shape", "Width", "Depth", "Height", "Sides", "Steps", "StepsByHeight", "StepHeight", "WallThickness", "CavityGap", "Floor", "Ceiling", "DoorFrame", "ArchHeight", "InnerRadius", "SweepAngle", "HoleRatio", "TubeSides"],
    "WhiteBoxLayerSettings": ["Name", "Visible", "Collision", "Tint", "CombineMode", "InvertNormals", "EdgesOnly", "Position", "Rotation", "Scale", "MirrorX", "MirrorY", "MirrorZ", "ArrayCount", "ArrayOffset"],
    "WhiteBoxPolygonInfo": ["Id", "FaceCount", "Normal", "Center", "Area", "VertexIds", "VertexPositions"],
    "WhiteBoxEdgeInfo": ["Id", "VertexA", "VertexB", "Start", "End", "Length", "Boundary", "Interior"],
    "WhiteBoxDisplaySettings": ["EdgesOnly", "UseGlobalTint", "Tint", "UseTexture", "MaterialPath", "CsgSolver", "FlipYZForExport"],
    "WhiteBoxBooleanSettings": ["SourceEntity", "Operation", "Live", "AffectActiveOnly", "SourceAfterApply", "ExcludeFromBoolean", "BooleanOthers", "CutterOperation"],
    "WhiteBoxDrawSettings": ["Shape", "Sides", "HoleRatio", "TubeSides", "Carve", "MergeUnion", "PolygonExtrude", "UnitCube", "UnitCubeSize", "UnitCubeShowGrid", "StairSteps", "StairByHeight", "StairStepHeight", "StairRotation"],
}


def _fields(struct):
    return _FIELDS[struct.typename]


def _snake(name):
    # Acronyms stay together: FlipYZForExport -> flip_yz_for_export
    return re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", name)).lower()


def _struct_to_dict(struct):
    return {_snake(n): _plain(getattr(struct, n)) for n in _fields(struct)}


def _fill_struct(struct, values, what):
    """Set the named fields of a scripted struct from a dict, coercing tuples, names and ids to the field's type."""
    known = {_norm(n): n for n in _fields(struct)}
    for key, value in values.items():
        field = known.get(_norm(key))
        if field is None:
            raise ValueError("%s has no setting '%s'; it has %s" % (what, key, sorted(_snake(n) for n in known.values())))
        if field in _ENUM_FIELDS:
            value = _enum(_ENUM_FIELDS[field], value, field)
        else:
            current = getattr(struct, field)
            kind = _typename(current)
            # The Editor silently ignores an int assigned to a float property, so numbers take the field's type.
            if isinstance(current, bool):
                value = bool(value)
            elif isinstance(current, float):
                value = float(value)
            elif isinstance(current, int):
                value = int(value)
            elif kind == "Vector3":
                value = _vec(value)
            elif kind == "Color":
                value = _color(value)
            elif kind == "EntityId":
                value = _eid(value) if value else entity.EntityId()
        setattr(struct, field, value)
    return struct


def _ids(value):
    """One id, a polygon / edge object, or any iterable of them, as a plain list of ints."""
    if value is None:
        return []
    if isinstance(value, int):
        return [int(value)]
    if hasattr(value, "id"):
        return [int(value.id)]
    return [int(v.id) if hasattr(v, "id") else int(v) for v in value]


def _eid(ref):
    """An EntityId from an EntityId, '[1234]', '1234' or an entity name."""
    if hasattr(ref, "IsValid"):
        return ref
    text = str(ref).strip()
    wanted = text if text.startswith("[") else "[%s]" % text if text.isdigit() else None
    found = list(entity.SearchBus(bus.Broadcast, "SearchEntities", entity.SearchFilter()) or [])
    if wanted:
        for eid in found:
            if eid.ToString() == wanted:
                return eid
    for eid in found:
        if editor.EditorEntityInfoRequestBus(bus.Event, "GetName", eid) == text:
            return eid
    raise ValueError("entity not found: %s" % text)


@contextlib.contextmanager
def undo(label):
    """Make every edit inside the block one undo step."""
    editor.ToolsApplicationRequestBus(bus.Broadcast, "BeginUndoBatch", "White Box: %s" % label)
    try:
        yield
    finally:
        editor.ToolsApplicationRequestBus(bus.Broadcast, "EndUndoBatch")


def _component_type_id():
    ids = editor.EditorComponentAPIBus(
        bus.Broadcast, "FindComponentTypeIdsByEntityType", ["White Box"], entity.EntityType().Game)
    if not ids or ids[0].IsNull():
        raise WhiteBoxError("the White Box component type is not registered; is the WhiteBox gem enabled?")
    return ids[0]


# ---------------------------------------------------------------- query results

class Polygon:
    """One polygon: id, outward normal, centre, area and its border (vertex ids with matching positions)."""

    def __init__(self, info):
        self.id = int(info.Id)
        self.face_count = int(info.FaceCount)
        self.normal = _tup(info.Normal)
        self.center = _tup(info.Center)
        self.area = float(info.Area)
        self.vertex_ids = [int(v) for v in info.VertexIds]
        self.positions = [_tup(p) for p in info.VertexPositions]

    def to_dict(self):
        return {"id": self.id, "face_count": self.face_count, "normal": self.normal, "center": self.center,
                "area": self.area, "vertex_ids": self.vertex_ids, "positions": self.positions}

    def __repr__(self):
        return "Polygon(id=%d, normal=(%.2f, %.2f, %.2f), area=%.3f)" % ((self.id,) + self.normal + (self.area,))


class Edge:
    """One edge: id, the two vertex ids, end positions, length and whether it borders a hole."""

    def __init__(self, info):
        self.id = int(info.Id)
        self.a = int(info.VertexA)
        self.b = int(info.VertexB)
        self.start = _tup(info.Start)
        self.end = _tup(info.End)
        self.length = float(info.Length)
        self.boundary = bool(info.Boundary)
        self.interior = bool(info.Interior)

    def to_dict(self):
        return {"id": self.id, "a": self.a, "b": self.b, "start": self.start, "end": self.end, "length": self.length,
                "boundary": self.boundary, "interior": self.interior}

    def __repr__(self):
        return "Edge(id=%d, %d-%d, length=%.3f)" % (self.id, self.a, self.b, self.length)


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


# ---------------------------------------------------------------- the entity wrapper

class WhiteBox:
    """A White Box entity driven through the authoring bus. Build one with create() or existing()."""

    def __init__(self, entity_id):
        self.id = _eid(entity_id)
        # An unhandled bus event answers None, so a count is also the proof that the component is active.
        if _BUS(bus.Event, "GetLayerCount", self.id) is None:
            raise WhiteBoxError("entity %s has no active White Box component" % self.id.ToString())

    # -- creation ---------------------------------------------------------------------------------------------------

    @classmethod
    def existing(cls, entity_ref):
        """Wrap a White Box entity that is already in the level (an entity id, '[1234]' or a name)."""
        return cls(entity_ref)

    @classmethod
    def create(cls, name="WhiteBox", position=None, rotation_deg=None, scale=None, parent=None, shape=None,
               blank=True, **shape_params):
        """Make an entity with a White Box component. With blank (the default) any starting geometry is removed;
        with shape, a parametric layer of that shape is added using shape_params (width, depth, height, sides, ...)."""
        with undo("Create %s" % name):
            parent_id = _eid(parent) if parent else entity.EntityId()
            eid = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", parent_id)
            if not eid.IsValid():
                raise WhiteBoxError("CreateNewEntity failed; is a level open?")
            editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
            outcome = editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [_component_type_id()])
            if not outcome.IsSuccess():
                raise WhiteBoxError("could not add the White Box component")
            if position is not None:
                components.TransformBus(bus.Event, "SetLocalTranslation", eid, _vec(position))
            if rotation_deg is not None:
                radians = [float(r) * 3.141592653589793 / 180.0 for r in rotation_deg]
                components.TransformBus(bus.Event, "SetLocalRotation", eid, _vec(radians))
            if scale is not None:
                components.TransformBus(bus.Event, "SetLocalUniformScale", eid, float(scale))
            box = cls(eid)
            if blank:
                box.clear()
            if shape is not None:
                box.add_shape(shape, **shape_params)
        return box

    # -- internals --------------------------------------------------------------------------------------------------

    def _call(self, name, *args):
        return _BUS(bus.Event, name, self.id, *args)

    def _result(self, what, result):
        if not result.Success:
            raise WhiteBoxError("%s: %s" % (what, result.Message))
        return [int(i) for i in result.Ids]

    def _layer(self, layer):
        return self.active_layer if layer is None else int(layer)

    # -- overview ---------------------------------------------------------------------------------------------------

    def describe(self):
        """Layers, mesh counts, bounds and the display / boolean / draw values, as a dict."""
        return json.loads(self._call("Describe"))

    def bounds(self):
        """((min), (max)) of the active layer's vertices, or None when it is empty."""
        points = list(self.vertices().values())
        if not points:
            return None
        lows = tuple(min(p[i] for p in points) for i in range(3))
        highs = tuple(max(p[i] for p in points) for i in range(3))
        return lows, highs

    # -- layers -----------------------------------------------------------------------------------------------------

    @property
    def layer_count(self):
        return int(self._call("GetLayerCount"))

    @property
    def active_layer(self):
        return int(self._call("GetActiveLayer"))

    def set_active_layer(self, index):
        """Make a layer the edit target; the mesh queries and edits act on it."""
        if not self._call("SetActiveLayer", int(index)):
            raise WhiteBoxError("layer %s does not exist" % index)

    def clear(self):
        """Remove every layer and any loose geometry, leaving an empty White Box."""
        for _ in range(self.layer_count):
            self._call("DeleteActiveLayer")
        self._call("ClearMesh")  # nothing to clear on an empty box is fine, so the outcome is ignored

    def add_layer(self):
        """Append an empty mesh layer and make it active; returns its index."""
        return int(self._call("AddLayer"))

    def add_shape(self, shape, **params):
        """Append a parametric shape layer (box, cylinder, pyramid, cone, sphere, staircase, room, door,
        circular_stairs, plane, torus, pipe, polygon) and make it active. params are shape_params() fields. Returns its index."""
        index = int(self._call("AddShapeLayer", _enum(SHAPES, shape, "shape")))
        if index < 0:
            raise WhiteBoxError("could not add a %s layer" % shape)
        if params:
            self.set_shape(index, **params)
        return index

    def shape_params(self, layer=None):
        """A parametric layer's values: width, depth, height, sides, steps, wall_thickness, ..."""
        return _struct_to_dict(self._call("GetShapeParams", self._layer(layer)))

    def set_shape(self, layer=None, **params):
        """Change a parametric layer's values; its mesh is rebuilt. Fails once the layer has been baked or edited."""
        layer = self._layer(layer)
        struct = _fill_struct(self._call("GetShapeParams", layer), params, "shape parameters")
        self._result("set_shape", self._call("SetShapeParams", layer, struct))

    def bake_shape(self, layer=None):
        """Freeze a parametric layer into an ordinary mesh. Any vertex or polygon edit does this on its own."""
        self._result("bake_shape", self._call("BakeShapeLayer", self._layer(layer)))

    def layer_settings(self, layer=None):
        """A layer's name, visibility, collision, tint, combine_mode, transform and mirror / array modifiers."""
        return _struct_to_dict(self._call("GetLayerSettings", self._layer(layer)))

    def set_layer(self, layer=None, **settings):
        """Change a layer's settings. combine_mode is separate, union, subtract or intersect (CSG against the layers below)."""
        layer = self._layer(layer)
        struct = _fill_struct(self._call("GetLayerSettings", layer), settings, "layer settings")
        self._result("set_layer", self._call("SetLayerSettings", layer, struct))

    def duplicate_layer(self):
        """Copy the active layer above itself; returns the active index afterwards."""
        return int(self._call("DuplicateActiveLayer"))

    def delete_layer(self):
        """Delete the active layer."""
        if not self._call("DeleteActiveLayer"):
            raise WhiteBoxError("there is no layer to delete")

    def move_layer(self, source, destination):
        """Reorder layers; the order is the order booleans accumulate in."""
        if not self._call("MoveLayer", int(source), int(destination)):
            raise WhiteBoxError("layer index out of range")

    def apply_layer_transform(self):
        """Bake the active layer's position, rotation and scale into its vertices."""
        self._result("apply_layer_transform", self._call("ApplyLayerTransform"))

    def apply_layer_modifiers(self):
        """Bake the active layer's mirror and array into its mesh."""
        self._result("apply_layer_modifiers", self._call("ApplyLayerModifiers"))

    # -- queries ----------------------------------------------------------------------------------------------------

    def vertices(self):
        """{vertex id: (x, y, z)} for the active layer."""
        ids = list(self._call("GetVertexIds"))
        positions = list(self._call("GetVertexPositions"))
        return {int(i): _tup(p) for i, p in zip(ids, positions)}

    def triangles(self):
        """[(vertex id, vertex id, vertex id)] for every face of the active layer, counter-clockwise."""
        flat = [int(i) for i in self._call("GetFaceVertexIds")]
        return [tuple(flat[i:i + 3]) for i in range(0, len(flat), 3)]

    def polygons(self):
        """[Polygon] for the active layer, ordered by id."""
        return [Polygon(info) for info in self._call("GetPolygons")]

    def edges(self, include_interior=False):
        """[Edge] for the active layer: polygon borders, plus the triangulation edges inside polygons when asked."""
        return [Edge(info) for info in self._call("GetEdges", bool(include_interior))]

    def facing(self, direction, min_dot=0.9):
        """Ids of the polygons whose normal points within acos(min_dot) of direction, e.g. (0, 0, 1) for upward faces."""
        length = _dot(direction, direction) ** 0.5 or 1.0
        unit = tuple(c / length for c in direction)
        return [p.id for p in self.polygons() if _dot(p.normal, unit) >= min_dot]

    def where(self, predicate):
        """Ids of the polygons for which predicate(Polygon) is true."""
        return [p.id for p in self.polygons() if predicate(p)]

    # -- geometry ---------------------------------------------------------------------------------------------------

    def set_mesh(self, positions, triangles):
        """Replace the active layer with an indexed triangle list (counter-clockwise faces outward). Coincident
        vertices weld and coplanar neighbours become polygons."""
        flat = [int(i) for tri in triangles for i in (tri if hasattr(tri, "__iter__") else (tri,))]
        self._result("set_mesh", self._call("SetMeshFromTriangles", [_vec(p) for p in positions], flat))

    def add_polygon(self, outline):
        """Add one convex polygon from an ordered outline of points; returns its polygon id."""
        return self._result("add_polygon", self._call("AddPolygon", [_vec(p) for p in outline]))[0]

    def clear_mesh(self):
        """Empty the active layer's mesh."""
        self._result("clear_mesh", self._call("ClearMesh"))

    def set_vertices(self, positions_by_id):
        """Move vertices to absolute positions: {vertex id: (x, y, z)}."""
        ids = [int(i) for i in positions_by_id]
        self._result("set_vertices", self._call(
            "SetVertexPositions", ids, [_vec(positions_by_id[i]) for i in positions_by_id]))

    def move_vertices(self, vertex_ids, offset):
        """Move vertices by an (x, y, z) offset."""
        self._result("move_vertices", self._call("TranslateVertices", _ids(vertex_ids), _vec(offset)))

    def extrude(self, polygons, distance):
        """Push polygons out along their normals as new geometry (negative digs in); returns the new polygon ids."""
        return self._result("extrude", self._call("ExtrudePolygons", _ids(polygons), float(distance)))

    def inset(self, polygons, fraction):
        """Shrink polygons into an inner polygon plus a border (fraction in (0, 1)); returns the inner polygon ids."""
        return self._result("inset", self._call("InsetPolygons", _ids(polygons), float(fraction)))

    def push(self, polygons, distance):
        """Slide existing polygons along their normals without creating faces."""
        return self._result("push", self._call("TranslatePolygons", _ids(polygons), float(distance)))

    def bevel(self, edges, width, segments=1, profile=0.5):
        """Bevel convex edges: width is the face offset, segments 1-32 rounds the profile."""
        self._result("bevel", self._call("BevelEdges", _ids(edges), float(width), int(segments), float(profile)))

    def bridge(self, polygons=(), edges=()):
        """Join two facing polygons, or two open boundary edges, with a strip of faces."""
        self._result("bridge", self._call("Bridge", _ids(polygons), _ids(edges)))

    def fill_hole(self, edges):
        """Cap the planar hole bordered by these open edges; returns the cap polygon id."""
        return self._result("fill_hole", self._call("FillHole", _ids(edges)))[0]

    def delete_polygons(self, polygons):
        """Delete polygons but keep their vertices."""
        self._result("delete_polygons", self._call("DeletePolygons", _ids(polygons)))

    def merge_polygons(self, polygons):
        """Merge connected polygons into one; returns its id."""
        return self._result("merge_polygons", self._call("MergePolygons", _ids(polygons)))[0]

    def weld(self, vertex_ids, at_last=False):
        """Merge vertices at their average position, or at the last one given."""
        self._result("weld", self._call("WeldVertices", _ids(vertex_ids), bool(at_last)))

    def loop_cut(self, edge, count=1, slide=0.0):
        """Cut count evenly spaced loops through the quad strip around an edge; slide in [-1, 1] shifts them."""
        self._result("loop_cut", self._call("InsertEdgeLoops", _ids(edge)[0], int(count), float(slide)))

    def subdivide(self, polygons):
        """Split each polygon into quads meeting at its centre; returns the new polygon ids."""
        return self._result("subdivide", self._call("SubdividePolygons", _ids(polygons)))

    def connect(self, vertex_ids):
        """Cut straight edges between vertices; returns the new edge ids."""
        return self._result("connect", self._call("ConnectVertices", _ids(vertex_ids)))

    def split_edge(self, edge, fraction=0.5):
        """Add a vertex on a polygon border edge at fraction (0, 1) from its first end; returns the vertex id."""
        return self._result("split_edge", self._call("InsertVertexOnEdge", _ids(edge)[0], float(fraction)))[0]

    def flip_edge(self, edge):
        """Flip the diagonal of the quad an edge sits in."""
        self._result("flip_edge", self._call("FlipEdge", _ids(edge)[0]))

    def detach(self, polygons):
        """Move polygons into a new layer above (it becomes active); returns that layer's index."""
        return self._result("detach", self._call("DetachPolygonsToLayer", _ids(polygons)))[0]

    def repair(self):
        """Weld coincident vertices and regroup coplanar faces so the mesh is a clean manifold."""
        self._result("repair", self._call("RepairMesh"))

    # -- surface ----------------------------------------------------------------------------------------------------

    def material(self, polygons, path):
        """Assign a material by product path (e.g. 'materials/brick.azmaterial'); '' resets to the entity default.
        An empty polygon list means every polygon."""
        self._result("material", self._call("SetPolygonMaterial", _ids(polygons), path or ""))

    def paint(self, polygons, rgba):
        """Face paint colour as (r, g, b[, a]) in 0-1; alpha 0 clears it. An empty polygon list means every polygon."""
        self._result("paint", self._call("SetPolygonPaint", _ids(polygons), _color(rgba)))

    def smooth(self, polygons, groups, mode="set"):
        """Smoothing-group bitmask (bit n is group n + 1); mode is set, add or remove. Empty polygons means all."""
        self._result("smooth", self._call(
            "SetPolygonSmoothing", _ids(polygons), int(groups), _enum(SMOOTH_EDITS, mode, "smoothing edit")))

    def auto_smooth(self, polygons=(), angle=30.0):
        """Smooth neighbours within angle degrees and keep sharper creases hard. Empty polygons means all."""
        self._result("auto_smooth", self._call("AutoSmooth", _ids(polygons), float(angle)))

    def uv(self, polygons=(), mode="planar", scale=(1.0, 1.0), offset=(0.0, 0.0), rotation=0.0):
        """Texture projection: mode world, planar or manual; scale is repeats per metre. Empty polygons means all."""
        self._result("uv", self._call(
            "SetPolygonUvProjection", _ids(polygons), _enum(UV_MODES, mode, "uv mode"),
            float(scale[0]), float(scale[1]), float(offset[0]), float(offset[1]), float(rotation)))

    def fit_uv(self, polygons=()):
        """Scale and offset each polygon's texture to span it once. Empty polygons means all."""
        self._result("fit_uv", self._call("FitPolygonUvs", _ids(polygons)))

    def recalc_uvs(self):
        """Recompute normals and planar UVs for the active layer."""
        self._result("recalc_uvs", self._call("RecalculateUvs"))

    # -- entity settings --------------------------------------------------------------------------------------------

    def display(self):
        """edges_only, use_global_tint, tint, use_texture, material_path, csg_solver, flip_yz_for_export."""
        return _struct_to_dict(self._call("GetDisplaySettings"))

    def set_display(self, **settings):
        """Change display values; csg_solver is manifold or fast, material_path a product path ('' for the built-in)."""
        struct = _fill_struct(self._call("GetDisplaySettings"), settings, "display settings")
        self._result("set_display", self._call("SetDisplaySettings", struct))

    def boolean(self):
        """source_entity, operation, live, affect_active_only, source_after_apply, exclude_from_boolean,
        boolean_others, cutter_operation."""
        return _struct_to_dict(self._call("GetBooleanSettings"))

    def set_boolean(self, **settings):
        """Change boolean values; source_entity takes an entity id or name, operations are union, subtraction, intersection."""
        struct = _fill_struct(self._call("GetBooleanSettings"), settings, "boolean settings")
        self._result("set_boolean", self._call("SetBooleanSettings", struct))

    def apply_boolean(self):
        """One-shot CSG between this entity and its boolean source, using the operation from set_boolean."""
        self._result("apply_boolean", self._call("ApplyBoolean"))

    def draw(self):
        """The Draw Shape tool's values (shape, sides, unit cube stamp, staircase options)."""
        return _struct_to_dict(self._call("GetDrawSettings"))

    def set_draw(self, **settings):
        """Change the Draw Shape tool's values. They steer the interactive tool only."""
        struct = _fill_struct(self._call("GetDrawSettings"), settings, "draw settings")
        self._result("set_draw", self._call("SetDrawSettings", struct))

    def voxels(self, cells, filled=True):
        """Fill (or clear) 1x1x1 cells given by integer (x, y, z) minimum corners."""
        self._result("voxels", self._call("SetVoxelCells", [_vec(c) for c in cells], bool(filled)))

    def clear_stamp(self):
        """Remove every cell the cube stamp placed."""
        self._result("clear_stamp", self._call("ClearCubeStamp"))

    def add_collision(self):
        """Make sure the entity has a White Box collider."""
        self._result("add_collision", self._call("AddCollision"))

    def refresh_global_booleans(self):
        """Recompute every White Box in the level against the current global cutters."""
        self._result("refresh_global_booleans", self._call("RefreshGlobalBooleans"))

    def export_obj(self, path):
        """Write the evaluated mesh (every layer, booleans applied) to an .obj file."""
        self._result("export_obj", self._call("ExportObj", str(path)))


# ---------------------------------------------------------------- MCP / batch entry points

_NOT_OPS = {"create", "existing", "run", "describe", "layer_count", "active_layer"}


def operations():
    """{op name: one-line help} for every method run() accepts."""
    table = {}
    for name, member in inspect.getmembers(WhiteBox, predicate=inspect.isfunction):
        if name.startswith("_") or name in _NOT_OPS:
            continue
        doc = (inspect.getdoc(member) or "").splitlines()
        table[name] = "%s%s: %s" % (name, str(inspect.signature(member)).replace("(self, ", "(").replace("(self)", "()"),
                                     doc[0] if doc else "")
    return table


def _jsonable(value):
    if hasattr(value, "to_dict"):
        return value.to_dict()
    if isinstance(value, dict):
        return {str(k): _jsonable(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [_jsonable(v) for v in value]
    return value


def run(entity_ref, ops, label="script"):
    """Run [{"op": "extrude", "polygons": [3], "distance": 0.5}, ...] on one White Box as a single undo step.
    Returns {"results": [...], "error": None | message}; it stops at the first failing op. Names come from operations()."""
    box = entity_ref if isinstance(entity_ref, WhiteBox) else WhiteBox.existing(entity_ref)
    allowed = operations()
    results = []
    error = None
    with undo(label):
        for index, op in enumerate(ops):
            op = dict(op)
            name = op.pop("op", None)
            if name not in allowed:
                error = "op %d: unknown op '%s'; choose from %s" % (index, name, sorted(allowed))
                break
            try:
                results.append(_jsonable(getattr(box, name)(**op)))
            except Exception as exc:  # report which op failed instead of losing the earlier results
                error = "op %d (%s): %s" % (index, name, exc)
                break
    return {"results": results, "error": error}
