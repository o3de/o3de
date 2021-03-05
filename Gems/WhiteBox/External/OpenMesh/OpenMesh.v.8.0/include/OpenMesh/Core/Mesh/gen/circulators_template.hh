//== CLASS DEFINITION =========================================================

	      
/** \class CirculatorT CirculatorsT.hh <OpenMesh/Mesh/Iterators/CirculatorsT.hh>
    Circulator.
*/

template <class Mesh>
class CirculatorT
{
 public:


  //--- Typedefs ---

  typedef typename Mesh::HalfedgeHandle   HalfedgeHandle;

  typedef TargetType           value_type;
  typedef TargetHandle         value_handle;

#if IsConst
  typedef const Mesh&         mesh_ref;
  typedef const Mesh*         mesh_ptr;
  typedef const TargetType&   reference;
  typedef const TargetType*   pointer;
#else
  typedef Mesh&               mesh_ref;
  typedef Mesh*               mesh_ptr;
  typedef TargetType&         reference;
  typedef TargetType*         pointer;
#endif



  /// Default constructor
  CirculatorT() : mesh_(0), active_(false) {}


  /// Construct with mesh and a SourceHandle
  CirculatorT(mesh_ref _mesh, SourceHandle _start) :
    mesh_(&_mesh), 
    start_(_mesh.halfedge_handle(_start)),
    heh_(start_),
    active_(false)
  { post_init; }


  /// Construct with mesh and start halfedge
  CirculatorT(mesh_ref _mesh, HalfedgeHandle _heh) :
    mesh_(&_mesh),
    start_(_heh),
    heh_(_heh),
    active_(false)
  { post_init; }


  /// Copy constructor
  CirculatorT(const CirculatorT& _rhs) :
    mesh_(_rhs.mesh_),
    start_(_rhs.start_),
    heh_(_rhs.heh_),
    active_(_rhs.active_)
  { post_init; }


  /// Assignment operator
  CirculatorT& operator=(const CirculatorT<Mesh>& _rhs)
  {
    mesh_   = _rhs.mesh_;
    start_  = _rhs.start_;
    heh_    = _rhs.heh_;
    active_ = _rhs.active_;
    return *this;
  }


#if IsConst
  /// construct from non-const circulator type
  CirculatorT(const NonConstCircT<Mesh>& _rhs) :
    mesh_(_rhs.mesh_),
    start_(_rhs.start_),
    heh_(_rhs.heh_),
    active_(_rhs.active_)
  { post_init; }


  /// assign from non-const circulator
  CirculatorT& operator=(const NonConstCircT<Mesh>& _rhs)
  {
    mesh_   = _rhs.mesh_;
    start_  = _rhs.start_;
    heh_    = _rhs.heh_;
    active_ = _rhs.active_;
    return *this;
  }
#else
  friend class ConstCircT<Mesh>;
#endif  


  /// Equal ?
  bool operator==(const CirculatorT& _rhs) const {
    return ((mesh_   == _rhs.mesh_) &&
	    (start_  == _rhs.start_) &&
	    (heh_    == _rhs.heh_) &&
	    (active_ == _rhs.active_));
  }


  /// Not equal ?
  bool operator!=(const CirculatorT& _rhs) const {
    return !operator==(_rhs);
  }


  /// Pre-Increment (next cw target)
  CirculatorT& operator++() { 
    assert(mesh_);
    active_ = true;
    increment;
    return *this;
  }


  /// Pre-Decrement (next ccw target)
  CirculatorT& operator--() { 
    assert(mesh_);
    active_ = true;
    decrement;
    return *this;
  }


  /** Get the current halfedge. There are \c Vertex*Iters and \c
      Face*Iters.  For both the current state is defined by the
      current halfedge. This is what this method returns. 
  */
  HalfedgeHandle current_halfedge_handle() const {
    return heh_;
  }


  /// Return the handle of the current target.
  TargetHandle handle() const {
    assert(mesh_);
    return get_handle; 
  }


  /// Cast to the handle of the current target.
  operator TargetHandle() const {
    assert(mesh_);
    return get_handle; 
  }
    

  ///  Return a reference to the current target.
  reference operator*() const { 
    assert(mesh_);
    return mesh_->deref(handle());
  }


  /// Return a pointer to the current target.
  pointer operator->() const { 
    assert(mesh_);
    return &mesh_->deref(handle());
  }


  /** Returns whether the circulator is still valid.
      After one complete round around a vertex/face the circulator becomes
      invalid, i.e. this function will return \c false. Nevertheless you
      can continue circulating. This method just tells you whether you
      have completed the first round.
   */
  operator bool() const { 
    return heh_.is_valid() && ((start_ != heh_) || (!active_));
  }


private:

  mesh_ptr         mesh_;
  HalfedgeHandle   start_, heh_;
  bool             active_;
};



