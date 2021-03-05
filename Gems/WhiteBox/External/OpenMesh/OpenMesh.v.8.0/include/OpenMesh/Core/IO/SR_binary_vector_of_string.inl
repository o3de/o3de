
template <> struct binary< std::vector< std::string > >
{
  // struct binary interface

  typedef std::vector< std::string > value_type;
  typedef value_type::value_type     elem_type;

  static const bool is_streamable = true;

  // Helper

  struct Sum
  {
    size_t operator() ( size_t _v1, const elem_type& _s2 )
    { return _v1 + binary<elem_type>::size_of(_s2); }
  };

  // struct binary interface

  static size_t size_of(void) { return UnknownSize; }

  static size_t size_of(const value_type& _v)
  { return std::accumulate( _v.begin(), _v.end(), size_t(0), Sum() ); }

  static 
  size_t store(std::ostream& _os, const value_type& _v, bool _swap=false)
  {
    return std::accumulate( _v.begin(), _v.end(), size_t(0), 
			    FunctorStore<elem_type>(_os, _swap) );
  }                                                        
                                                             
  static
  size_t restore(std::istream& _is, value_type& _v, bool _swap=false) 
  {
    return std::accumulate( _v.begin(), _v.end(), size_t(0), 
			    FunctorRestore<elem_type>(_is, _swap) );
  }                                                        
};
