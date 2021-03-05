//== CLASS DEFINITION =========================================================

	      
/** \class IteratorT IteratorsT.hh <OpenMesh/Mesh/Iterators/IteratorsT.hh>
    Linear iterator.
*/

template <class Mesh>
class IteratorT
{
public:
  

  //--- Typedefs ---

  typedef TargetType           value_type;
  typedef TargetHandle         value_handle;

#if IsConst
  typedef const value_type&    reference;
  typedef const value_type*    pointer;
  typedef const Mesh*          mesh_ptr;
  typedef const Mesh&          mesh_ref;
#else
  typedef value_type&          reference;
  typedef value_type*          pointer;
  typedef Mesh*                mesh_ptr;
  typedef Mesh&                mesh_ref;
#endif




  /// Default constructor.
  IteratorT() 
    : mesh_(0), skip_bits_(0) 
  {}


  /// Construct with mesh and a target handle. 
  IteratorT(mesh_ref _mesh, value_handle _hnd, bool _skip=false) 
    : mesh_(&_mesh), hnd_(_hnd), skip_bits_(0) 
  {
    if (_skip) enable_skipping();
  }


  /// Copy constructor
  IteratorT(const IteratorT& _rhs) 
    : mesh_(_rhs.mesh_), hnd_(_rhs.hnd_), skip_bits_(_rhs.skip_bits_)
  {}
  

  /// Assignment operator
  IteratorT& operator=(const IteratorT<Mesh>& _rhs) 
  {
    mesh_      = _rhs.mesh_;
    hnd_       = _rhs.hnd_;
    skip_bits_ = _rhs.skip_bits_;
    return *this;
  }


#if IsConst

  /// Construct from a non-const iterator
  IteratorT(const NonConstIterT<Mesh>& _rhs) 
    : mesh_(_rhs.mesh_), hnd_(_rhs.hnd_), skip_bits_(_rhs.skip_bits_) 
  {}
  

  /// Assignment from non-const iterator
  IteratorT& operator=(const NonConstIterT<Mesh>& _rhs) 
  {
    mesh_      = _rhs.mesh_;
    hnd_       = _rhs.hnd_;
    skip_bits_ = _rhs.skip_bits_;
    return *this;
  }

#else
  friend class ConstIterT<Mesh>;
#endif


  /// Standard dereferencing operator.
  reference operator*()  const { return mesh_->deref(hnd_); }
  
  /// Standard pointer operator.
  pointer   operator->() const { return &(mesh_->deref(hnd_)); }
  
  /// Get the handle of the item the iterator refers to.
  value_handle handle() const { return hnd_; }

  /// Cast to the handle of the item the iterator refers to.
  operator value_handle() const { return hnd_; }

  /// Are two iterators equal? Only valid if they refer to the same mesh!
  bool operator==(const IteratorT& _rhs) const 
  { return ((mesh_ == _rhs.mesh_) && (hnd_ == _rhs.hnd_)); }

  /// Not equal?
  bool operator!=(const IteratorT& _rhs) const 
  { return !operator==(_rhs); }
  
  /// Standard pre-increment operator
  IteratorT& operator++() 
  { hnd_.__increment(); if (skip_bits_) skip_fwd(); return *this; }

  /// Standard pre-decrement operator
  IteratorT& operator--() 
  { hnd_.__decrement(); if (skip_bits_) skip_bwd(); return *this; }
  

  /// Turn on skipping: automatically skip deleted/hidden elements
  void enable_skipping()
  {
    if (mesh_ && mesh_->has_element_status())
    {
      Attributes::StatusInfo status;
      status.set_deleted(true);
      status.set_hidden(true);
      skip_bits_ = status.bits();
      skip_fwd();
    }
    else skip_bits_ = 0;
  }


  /// Turn on skipping: automatically skip deleted/hidden elements
  void disable_skipping() { skip_bits_ = 0; }



private:

  void skip_fwd() 
  {
    assert(mesh_ && skip_bits_);
    while ((hnd_.idx() < (signed) mesh_->n_elements()) && 
	   (mesh_->status(hnd_).bits() & skip_bits_))
      hnd_.__increment();
  }


  void skip_bwd() 
  {
    assert(mesh_ && skip_bits_);
    while ((hnd_.idx() >= 0) && 
	   (mesh_->status(hnd_).bits() & skip_bits_))
      hnd_.__decrement();
  }



private:
  mesh_ptr      mesh_;
  value_handle  hnd_;
  unsigned int  skip_bits_;
};


