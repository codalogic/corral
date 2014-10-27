returned
========

returned is another class for combining return code and exception based
programming with the aim of making coding safer.

returned employs the following assumptions:

- returned is intended to track small resource handles rather than large objects.
  (The example illustrates working with a FILE * handles for files.)

- Currently, it only returns whether the resource handle is valid or not.
  There is no indication of 'why' it may have not been possible to obtain
  a resource handle.  This may change in future.
  
- If you don't access the resource then it's not an error to not look 
  at the error condition.

- Similar to std::auto_ptr and std::unique_ptr, the returned object is
  responsible for cleaning up the resource unless you call the release()
  method.

- returned's behaviour is customised by template specialisation of
  a returned_config<> class.  This allows specifying how to tell whether a 
  resource handle is valid, how a resource should be cleaned up, and what the
  default exception should be.

- returned allows the _calling_ function to decide which exception is thrown
  if an invalid resource is queried.

The latter point of allowing a _calling_ function to set the exception opens up
an alternative way of coding.  For example:

```cpp
returned<FILE *> open_file( const char * name, const char * mode )
{
    return returned<FILE *>( fopen( name, mode ) );
}

class bad_file_in_1 : public bad_returned_file {};
class bad_file_in_2 : public bad_returned_file {};

void process_files()
{
    try
    {
        returned<FILE *, bad_file_in_1> fin2_1( open_file( "test-exists.txt", "r" ) );
        FILE * f1 = fin2_1.get();	// Will throw bad_file_in_1 if file not opened

        returned<FILE *, bad_file_in_2> fin2_2( open_file( "test-not-exists.txt", "r" ) );
        FILE * f2 = fin2_2.get();	// Will throw bad_file_in_2 if file not opened
        
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

Because `returned` has been configured to know how to close a file there is no
resource leak if, say, the first file is opened successfully and the second one
isn't.

The template specialisation of returned_config for a FILE * handle 
(for example) can look like this:

```cpp
class bad_returned_file : public bad_returned {};

namespace ret {
template<>
struct returned_config< FILE * >
{
    typedef FILE * value_t;
    static bool validator( const value_t & f ) { return f != 0; }
    static void on_reset( value_t & f ) { if( f ) fclose( f ); }
    typedef bad_returned_file Texception;
};
}	// namespace ret
```

Where:

- validator() is used to tell whether a resource is valid.

- on_reset() tells how to release a resource.

Note that, in the following line from above:

```cpp
    return returned<FILE *>( fopen( name, mode ) );
```

returned_config< FILE * >::validator() is called to test whether the
file returned by fopen() is open.

To make sure that the default `returned_config` template is not used
instead of a customised version (for example, due to a missed #include
file), the default template version of `returned_config` does not compile.
All handle types must therefore have a custom template defined for them.  If no
specialising is required, then a pre-defined default can be used by doing:

```cpp
namespace ret {
template<>
struct returned_config<int> : public returned_config_simple<int> {};
}
```

The error-code method of coding can also be supported.  For example:

```cpp
void file_example_3();

void my_program()
{
    try
    {
        file_handler();
    }
    catch( bad_returned & )
    {
        error( "A resource was invalid and not checked" );
    }
}

void file_handler()
{
        returned<FILE *> fin3_1( open_file( "test-in-1.txt", "r" ) );
        if( fin3_1.is_valid() )
        {
            returned<FILE *> fin3_2 = open_file( "test-in-2.txt", "r" );
            if( fin3_2.is_valid() )
            {
                // Use fin3_1 and fin3_2
            }
        }
}
```

Further methods of interest are:

- `returned()`: Construct object with resource marked as invalid.

- `returned( Tvalue value )`: Construct object using resource handle
  value.  `returned_config<Tvalue>::validator()` will be called to
  determine whether the resource is valid.

- `returned( validator_t validator, Tvalue value )`: Construct object
  using resource handle value.  The function pointed to by `validator`
  will be called to determine whether the resource is valid.

- `is_valid()`: Return true if the resource is valid, false if not.

- `check()`: Throw the exception if the resource is invalid, otherwise
  do nothing.

- `get()`: Return the handle if valid, or throw the exception.

- `release()`: If the resource is valid, it will return the resource
  and relinquish it's responsibility to clean up the resource when the
  'returned' object is destructed.  If invalid, it will throw the 
  exception.

- `reset()`: Clean up the resource immediately.

Installation and The Repository
===============================

The main library code is in the file `returned.h`.  This is what you 
need if you want to include the code in your project.

`returned-example.cpp` illustrates how the code can be used.
`annotate-lite.h` just contains simple code used for annotating the
example.

Implementation Notes
====================

In a similar vain to std::auto_ptr, only one `returned` object can own a
resource. (Hence assignment is disabled.)  This means, when returning
a `returned` object from a function ownership must be transferred between
the object in the called function and the object in the calling function.
To achieve this, the Colvin/Gibbons idiom (auto_ptr / auto_ptr_ref shuffle)
is used, which is why there is more compexity in the interface than you might
expect.

See Also
========

There are many other attempts in this area.  For example, see:

- Andrei Alexandrescu's [Expected<T>](http://www.hyc.io/boost/expected-proposal.pdf "Expected<T>")
