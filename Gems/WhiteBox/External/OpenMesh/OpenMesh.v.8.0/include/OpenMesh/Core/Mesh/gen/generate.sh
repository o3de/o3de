#!/bin/bash

#------------------------------------------------------------------------------


# generate_iterator( TargetType, n_elements, has_element_status )
function generate_iterator
{
    NonConstIter=$1"IterT"
    ConstIter="Const"$NonConstIter
    TargetType="typename Mesh::"$1
    TargetHandle="typename Mesh::"$1"Handle"


    cat iterators_template.hh \
    | sed -e "s/IteratorT/$NonConstIter/; s/IteratorT/$NonConstIter/;
	      s/NonConstIterT/$NonConstIter/;
	      s/ConstIterT/$ConstIter/;
              s/TargetType/$TargetType/;
	      s/TargetHandle/$TargetHandle/;
	      s/IsConst/0/;
	      s/n_elements/$2/;
	      s/has_element_status/$3/;"


    cat iterators_template.hh \
    | sed -e "s/IteratorT/$ConstIter/; s/IteratorT/$ConstIter/;
	      s/NonConstIterT/$NonConstIter/;
	      s/ConstIterT/$ConstIter/;
              s/TargetType/$TargetType/;
	      s/TargetHandle/$TargetHandle/;
	      s/IsConst/1/;
	      s/n_elements/$2/;
	      s/has_element_status/$3/;"
}


#------------------------------------------------------------------------------


# generate_circulator( NonConstName, SourceType, TargetType, 
#                      post_init,
#                      increment, decrement,
#                      get_handle,
#                      [Name] )
function generate_circulator
{
    NonConstCirc=$1
    ConstCirc="Const"$NonConstCirc
    SourceHandle="typename Mesh::"$2"Handle"
    TargetHandle="typename Mesh::"$3"Handle"
    TargetType="typename Mesh::"$3


    cat circulators_template.hh \
    | sed -e "s/CirculatorT/$NonConstCirc/; s/CirculatorT/$NonConstCirc/;
	      s/NonConstCircT/$NonConstCirc/;
	      s/ConstCircT/$ConstCirc/;
	      s/SourceHandle/$SourceHandle/;
	      s/TargetHandle/$TargetHandle/;
              s/TargetType/$TargetType/;
	      s/IsConst/0/;
              s/post_init/$4/;
              s/increment/$5/;
              s/decrement/$6/;
	      s/get_handle/$7/;"


    cat circulators_template.hh \
    | sed -e "s/CirculatorT/$ConstCirc/; s/CirculatorT/$ConstCirc/;
	      s/NonConstCircT/$NonConstCirc/;
	      s/ConstCircT/$ConstCirc/;
	      s/SourceHandle/$SourceHandle/;
	      s/TargetHandle/$TargetHandle/;
              s/TargetType/$TargetType/;
	      s/IsConst/1/;
              s/post_init/$4/;
              s/increment/$5/;
              s/decrement/$6/;
	      s/get_handle/$7/;"
}


#------------------------------------------------------------------------------


### Generate IteratorsT.hh

cat iterators_header.hh > IteratorsT.hh

generate_iterator Vertex   n_vertices   has_vertex_status    >> IteratorsT.hh
generate_iterator Halfedge n_halfedges  has_halfedge_status  >> IteratorsT.hh
generate_iterator Edge     n_edges      has_edge_status      >> IteratorsT.hh
generate_iterator Face     n_faces      has_face_status      >> IteratorsT.hh

cat footer.hh >> IteratorsT.hh


#------------------------------------------------------------------------------


### Generate CirculatorsT.hh

cat circulators_header.hh > CirculatorsT.hh


generate_circulator VertexVertexIterT Vertex Vertex \
    " " \
    "heh_=mesh_->cw_rotated_halfedge_handle(heh_);" \
    "heh_=mesh_->ccw_rotated_halfedge_handle(heh_);" \
    "mesh_->to_vertex_handle(heh_);" \
    >> CirculatorsT.hh

generate_circulator VertexOHalfedgeIterT Vertex Halfedge \
    " " \
    "heh_=mesh_->cw_rotated_halfedge_handle(heh_);" \
    "heh_=mesh_->ccw_rotated_halfedge_handle(heh_);" \
    "heh_" \
    >> CirculatorsT.hh

generate_circulator VertexIHalfedgeIterT Vertex Halfedge \
    " " \
    "heh_=mesh_->cw_rotated_halfedge_handle(heh_);" \
    "heh_=mesh_->ccw_rotated_halfedge_handle(heh_);" \
    "mesh_->opposite_halfedge_handle(heh_)" \
    >> CirculatorsT.hh

generate_circulator VertexEdgeIterT Vertex Edge \
    " " \
    "heh_=mesh_->cw_rotated_halfedge_handle(heh_);" \
    "heh_=mesh_->ccw_rotated_halfedge_handle(heh_);" \
    "mesh_->edge_handle(heh_)" \
    >> CirculatorsT.hh

generate_circulator VertexFaceIterT Vertex Face \
    "if (heh_.is_valid() \&\& !handle().is_valid()) operator++();" \
    "do heh_=mesh_->cw_rotated_halfedge_handle(heh_);  while ((*this) \&\& (!handle().is_valid()));" \
    "do	heh_=mesh_->ccw_rotated_halfedge_handle(heh_); while ((*this) \&\& (!handle().is_valid()));" \
    "mesh_->face_handle(heh_)" \
    >> CirculatorsT.hh


generate_circulator FaceVertexIterT Face Vertex \
    " " \
    "heh_=mesh_->next_halfedge_handle(heh_);" \
    "heh_=mesh_->prev_halfedge_handle(heh_);" \
    "mesh_->to_vertex_handle(heh_)" \
    >> CirculatorsT.hh

generate_circulator FaceHalfedgeIterT Face Halfedge \
    " " \
    "heh_=mesh_->next_halfedge_handle(heh_);" \
    "heh_=mesh_->prev_halfedge_handle(heh_);" \
    "heh_" \
    >> CirculatorsT.hh

generate_circulator FaceEdgeIterT Face Edge \
    " " \
    "heh_=mesh_->next_halfedge_handle(heh_);" \
    "heh_=mesh_->prev_halfedge_handle(heh_);" \
    "mesh_->edge_handle(heh_)" \
    >> CirculatorsT.hh

generate_circulator FaceFaceIterT Face Face \
    "if (heh_.is_valid() \&\& !handle().is_valid()) operator++();" \
    "do heh_=mesh_->next_halfedge_handle(heh_); while ((*this) \&\& (!handle().is_valid()));" \
    "do heh_=mesh_->prev_halfedge_handle(heh_); while ((*this) \&\& (!handle().is_valid()));" \
    "mesh_->face_handle(mesh_->opposite_halfedge_handle(heh_))" \
    >> CirculatorsT.hh


cat footer.hh >> CirculatorsT.hh


#------------------------------------------------------------------------------
