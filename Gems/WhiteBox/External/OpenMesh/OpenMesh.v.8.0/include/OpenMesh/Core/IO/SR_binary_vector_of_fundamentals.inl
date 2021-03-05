
#define BINARY_VECTOR( T ) \
template <> struct binary< std::vector< T > > {                 \
  typedef std::vector< T >       value_type;                    \
  typedef value_type::value_type elem_type;                     \
                                                                \
  static const bool is_streamable = true;                       \
                                                                \
  static size_t size_of(void)                                   \
  { return IO::UnknownSize; }                                   \
                                                                \
  static size_t size_of(const value_type& _v)                   \
  { return sizeof(elem_type)*_v.size(); }                       \
                                                                \
  static                                                        \
  size_t store(std::ostream& _os, const value_type& _v, bool _swap=false) { \
    size_t bytes=0;                                             \
                                                                \
    if (_swap)                                                  \
        bytes = std::accumulate( _v.begin(), _v.end(), bytes,   \
			  FunctorStore<elem_type>(_os,_swap) ); \
    else {                                                      \
      bytes = size_of(_v);                                       \
      _os.write( reinterpret_cast<const char*>(&_v[0]), bytes ); \
    }                                                            \
    return _os.good() ? bytes : 0;                              \
  }                                                             \
                                                                \
  static size_t restore(std::istream& _is, value_type& _v, bool _swap=false) { \
    size_t bytes=0;                                             \
                                                                \
    if ( _swap)                                                 \
      bytes = std::accumulate( _v.begin(), _v.end(), size_t(0),         \
			       FunctorRestore<elem_type>(_is, _swap) );           \
    else                                                        \
    {                                                           \
      bytes = size_of(_v);                                      \
      _is.read( reinterpret_cast<char*>(&_v[0]), bytes );       \
    }                                                           \
    return _is.good() ? bytes : 0;                              \
  }                                                             \
}

BINARY_VECTOR( short  );
BINARY_VECTOR( unsigned short  );
BINARY_VECTOR( int  );
BINARY_VECTOR( unsigned int  );
BINARY_VECTOR( long );
BINARY_VECTOR( unsigned long );
BINARY_VECTOR( float  );
BINARY_VECTOR( double );

#undef BINARY_VECTOR
