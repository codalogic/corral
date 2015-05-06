corral
========

corral started life as another class for combining return code and exception based
programming with the aim of making coding safer.  However, it can be used more
generally for corralling resources that need to be checked for proper initialisation
before being used, and so it has been given a more general name.

corral employs the following assumptions:

- corral is intended to track small resource handles rather than large objects.
  (The example illustrates working with a FILE * handles for files.)

- corral's behaviour is customised by template specialisation of
  a corral_config<> class.  This allows specifying how to tell whether a
  resource handle is valid, how a resource should be cleaned up, and what the
  default exception should be.

- Similar to std::auto_ptr and std::unique_ptr, the corral object is
  responsible for cleaning up the resource unless you call the release()
  method.

- If you don't access the resource then it's not an error to not look
  at the error condition.

- corral allows the _calling_ function to decide which exception is thrown
  if an invalid resource is queried.

- Currently, it only returns whether the resource handle is valid or not.
  There is no indication of 'why' it may have not been possible to obtain
  a resource handle.  This may change in future.

The latter point of allowing a _calling_ function to set the exception opens up
an alternative way of coding.  For example:

```cpp
corral<FILE *> open_file( const char * name, const char * mode )
{
    return corral<FILE *>( fopen( name, mode ) );
}

class bad_file_in_1 : public bad_corral_file {};
class bad_file_in_2 : public bad_corral_file {};

void process_files()
{
    try
    {
        corral<FILE *, bad_file_in_1> fin2_1( open_file( "test-exists.txt", "r" ) );
        FILE * f1 = fin2_1.get();   // Will throw bad_file_in_1 if file not opened

        corral<FILE *, bad_file_in_2> fin2_2( open_file( "test-not-exists.txt", "r" ) );
        FILE * f2 = fin2_2.get();   // Will throw bad_file_in_2 if file not opened

        // Use f1 and f2
    }
    catch( bad_file_in_1 & )
    {
        error( "Could not open test-exists.txt" );
    }
    catch( bad_file_in_2 & )
    {
        error( "Could not open test-not-exists.txt" );
    }
}
```

This removes a lot of tab nesting that traditionally occurs when accessing
multiple resources.

Because `corral` has been configured to know how to close a file there is no
resource leak if, say, the first file is opened successfully and the second one
isn't.

The template specialisation of corral_config for a FILE * handle
(for example) can look like this:

```cpp
class bad_corral_file : public bad_corral {};

namespace ret {
template<>
struct corral_config< FILE * >
{
    typedef FILE * value_t;
    static bool validator( const value_t & f ) { return f != 0; }
    static void on_reset( value_t & f ) { if( f ) fclose( f ); }
    typedef bad_corral_file Texception;
};
}   // namespace ret
```

Where:

- validator() is used to tell whether a resource is valid.

- on_reset() tells how to release a resource.

Note that, in the following line from above:

```cpp
    return corral<FILE *>( fopen( name, mode ) );
```

corral_config< FILE * >::validator() is called to test whether the
file corral by fopen() is open.

corral_config<T>::value_t allows for some indirection between the
'tag' type used to select the configuration and the actual type
of the `corral`'s value.  For example, if you had multiple handles
that had `int` type, you could have a `corral_config` of:

```cpp
class foo {};

namespace ret {
template<>
struct corral_config< foo >
{
    typedef int value_t;
    static bool validator( const value_t & f ) { return f != 0; }
    static void on_reset( value_t & f )
    {
        std::cout << "Yippee, we've closed\n";
    }
    typedef bad_corral_foo Texception;
};
}   // namespace ret
```

If you have a class of handles that have different types, but are all
share in the same way for validation checking and clean-up, for example,
windows handles of different kinds, then you can have a `corral_config`
of the form:

```cpp
template< typename Tvalue >
class whandle {};

namespace ret {
template<typename Tvalue>
struct corral_config< whandle<Tvalue> >
{
    typedef Tvalue value_t;
    static bool validator( const value_t & f ) { return f >= 0; }
    static void on_reset( value_t & f )
    {
        std::cout << "whandle<T> has been closed\n";
    }
    typedef bad_corral_whandle Texception;
};
}   // namespace ret
```

And use it as, for example:

```cpp
    corral<whandle<wnd>, bad_corral_custom_whandle> w( new_window() );
```

where `bad_corral_custom_whandle` is the exception to be thrown.

To make sure that the default `corral_config` template is not used
instead of a customised version (for example, due to a missed #include
file), the default template version of `corral_config` does not compile.
All handle types must therefore have a custom template defined for them.  If no
specialising is required, then a pre-defined default can be used by doing:

```cpp
namespace ret {
template<>
struct corral_config<int> : public corral_config_simple<int> {};
}
```

The error-code method of coding can also be supported.  For example:

```cpp
void file_handler();

void my_program()
{
    try
    {
        file_handler();
    }
    catch( bad_corral & )
    {
        error( "A resource was invalid and not checked" );
    }
}

void file_handler()
{
        corral<FILE *> fin3_1( open_file( "test-in-1.txt", "r" ) );
        if( fin3_1.is_valid() )
        {
            corral<FILE *> fin3_2 = open_file( "test-in-2.txt", "r" );
            if( fin3_2.is_valid() )
            {
                // Use fin3_1 and fin3_2
            }
        }
}
```

Further methods of interest are:

- `corral()`: Construct object with resource marked as invalid.

- `corral( Tvalue value )`: Construct object using resource handle
  value.  `corral_config<Tvalue>::validator()` will be called to
  determine whether the resource is valid.

- `corral( validator_t validator, Tvalue value )`: Construct object
  using resource handle value.  The function pointed to by `validator`
  will be called to determine whether the resource is valid.

- `is_valid()`: Return true if the resource is valid, false if not.

- `check()`: Throw the exception if the resource is invalid, otherwise
  do nothing.

- `get()`: Return the handle if valid, or throw the exception.

- `take( corral<...> & rhs )`: Take ownership of the resource owned
  by `rhs` (if possible).

- `release()`: If the resource is valid, it will return the resource
  and relinquish it's responsibility to clean up the resource when the
  'corral' object is destructed.  If invalid, it will throw the
  exception.

- `reset()`: Clean up the resource immediately.

Installation and The Repository
===============================

The main library code is in the file `corral.h`.  This is what you
need if you want to include the code in your project.

`corral-example.cpp` illustrates how the code can be used.
`annotate-lite.h` just contains simple code used for annotating the
example.

The code is targetted at C++03 and has been tested on VS2008, g++ 4.1.1
and g++ 4.7.0.

Implementation Notes
====================

In a similar vain to std::auto_ptr, only one `corral` object can own a
resource. (Hence assignment is disabled.)  This means, when returning
a `corral` object from a function ownership must be transferred between
the object in the called function and the object in the calling function.
To achieve this using C++03, the Colvin/Gibbons idiom (auto_ptr / auto_ptr_ref shuffle)
is used, which is why there is more compexity in the interface than you might
expect.  C++11 move semantics would make life easier.

Future Work
===========

Currently, whether a resource is valid or not is a simple Boolean condition.  In
future it may be desirable to have a finer grained definition of why a resource
may not be valid.  This will likely involve an error_t being added to corral_config<>
along with some kind of corral_config<>::is_valid() method.  For this reason
corral<>::m_is_valid and corral<>::m_is_owned are kept separate despite them so far
always being used together.

See Also
========

There are many other offerings in this area.  For example, see:

- Andrei Alexandrescu's [Expected<T>](http://www.hyc.io/boost/expected-proposal.pdf "Expected<T>")
