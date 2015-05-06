//----------------------------------------------------------------------------
// Copyright (c) 2014, Codalogic Ltd (http://www.codalogic.com)
// All rights reserved.
//
// The license for this file is based on the BSD-3-Clause license
// (http://www.opensource.org/licenses/BSD-3-Clause).
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// - Neither the name Codalogic Ltd nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------

#include "corral.h"

#include "annotate-lite.h"

#include <cstdio>

#if defined(_MSC_VER)
// Require error when nonstandard extension used :
//      'token' : conversion from 'type' to 'type'
//      e.g. reject foo( /*no const*/ foo & ) copy constructor
#pragma warning(error: 4239)
#endif

using namespace crrl;

namespace crrl {
template<>
struct corral_config<int> : public corral_config_simple<int> {};
}

void simple_no_value_set_example()
{
    try
    {
        corral<int> r;
        int t = r.get();
        Bad( "simple_no_value_set_example didn't throw" );
    }
    catch( bad_corral & )
    {
        Good( "simple_no_value_set_example threw" );
    }
    catch( ... )
    {
        Bad( "simple_no_value_set_example threw wrong exception" );
    }
}

void simple_value_set_example()
{
    try
    {
        corral<int> r( 1 );
        int t = r.get();
        Good( "simple_value_set_example didn't throw" );
    }
    catch( bad_corral & )
    {
        Bad( "simple_value_set_example threw" );
    }
    catch( ... )
    {
        Bad( "simple_value_set_example threw wrong exception" );
    }
}

void const_simple_value_set_example()
{
    try
    {
        const corral<int> r( 1 );
        int t = r.get();
        Good( "const_simple_value_set_example didn't throw" );
    }
    catch( bad_corral & )
    {
        Bad( "const_simple_value_set_example threw" );
    }
    catch( ... )
    {
        Bad( "const_simple_value_set_example threw wrong exception" );
    }
}

class bad_alternate_corral : public bad_corral {};

corral<int> my_op()
{
    return corral<int>();
}

void alternate_exception_example()
{
    try
    {
        corral<int, bad_alternate_corral> r( my_op() );
        int t = r.get();
        Bad( "alternate_exception_example didn't throw" );
    }
    catch( bad_alternate_corral & )
    {
        Good( "alternate_exception_example threw bad_alternate_corral" );
    }
    catch( bad_corral & )
    {
        Bad( "alternate_exception_example threw bad_corral" );
    }
    catch( ... )
    {
        Bad( "alternate_exception_example threw wrong exception" );
    }
}

template< typename T >
bool not_zero( const T & v )
{
    return v != 0;
}

corral<int> my_validated_op( int v )
{
    return corral<int>( v, not_zero );
}

void validated_non_throw_example()
{
    try
    {
        corral<int> r( my_validated_op( 1 ) );
        int t = r.get();
        Good( "validated_non_throw_example didn't throw" );
    }
    catch( bad_corral & )
    {
        Bad( "validated_non_throw_example threw" );
    }
    catch( ... )
    {
        Bad( "validated_non_throw_example threw wrong exception" );
    }
}

void validated_throw_example()
{
    try
    {
        corral<int> r( my_validated_op( 0 ) );
        int t = r.get();
        Bad( "validated_throw_example didn't throw" );
    }
    catch( bad_corral & )
    {
        Good( "validated_throw_example threw" );
    }
    catch( ... )
    {
        Bad( "validated_throw_example threw wrong exception" );
    }
}

class bad_corral_file : public bad_corral {};

namespace crrl {
template<>
struct corral_config< FILE * > : public corral_config_simple< FILE * >
{
    typedef FILE * value_t;
    static bool validator( const value_t & f ) { return f != 0; }
    static void on_reset( value_t & f )
    {
        Good( "file_corral on_reset called" );
        if( f )
            fclose( f );
    }
    typedef bad_corral_file Texception;
};
}   // namespace crrl

class bad_file_in_1 : public bad_corral_file {};
class bad_file_in_2 : public bad_corral_file {};

void file_example_1()
{
    try
    {
        corral<FILE *, bad_file_in_1> fin1_1( fopen( "test-exists.txt", "r" ) );
        fin1_1.check();
        corral<FILE *, bad_file_in_2> fin1_2( fopen( "test-not-exists.txt", "r" ) );
        FILE * f2 = fin1_2.get();
    }
    catch( bad_file_in_1 & )
    {
        Bad( "fin1_1 not open" );
    }
    catch( bad_file_in_2 & )
    {
        Good( "fin1_2 not open" );
    }
    catch( bad_corral & )
    {
        Bad( "file_example_1 threw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown file_example_1 exception thrown" );
    }
}

corral<FILE *> open_file( const char * name, const char * mode )
{
    return corral<FILE *>( fopen( name, mode ) );
}

void file_example_2()
{
    try
    {
        corral<FILE *, bad_file_in_1> fin2_1( open_file( "test-exists.txt", "r" ) );
        fin2_1.check();
        corral<FILE *, bad_file_in_2> fin2_2( open_file( "test-not-exists.txt", "r" ) );
        FILE * f2 = fin2_2.get();
    }
    catch( bad_file_in_1 & )
    {
        Bad( "fin2_1 not open" );
    }
    catch( bad_file_in_2 & )
    {
        Good( "fin2_2 not open" );
    }
    catch( bad_corral & )
    {
        Bad( "file_example_1 threw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown file_example_1 exception thrown" );
    }
}

void file_example_3()
{
    try
    {
        corral<FILE *> fin3_1( open_file( "test-in-1.txt", "r" ) );
        if( fin3_1.is_valid() )
        {
            corral<FILE *> fin3_2 = open_file( "test-in-2.txt", "r" );
            if( fin3_2.is_valid() )
            {
            }
        }
        Good( "file_example_2 didn't throw" );
    }
    catch( bad_corral & )
    {
        Bad( "file_example_2 shouldn't throw" );
    }
    catch( ... )
    {
        Bad( "Unknown file_example_2 exception thrown" );
    }
}

void file_default_exception()
{
    try
    {
        corral<FILE *> fin3_1( open_file( "test-in-1.txt", "r" ) );
        fin3_1.check();
        Bad( "file_default_exception didn't throw" );
    }
    catch( bad_corral_file & )
    {
        Good( "file_default_exception threw bad_corral_file" );
    }
    catch( bad_corral & )
    {
        Bad( "file_default_exception shouldn't throw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown file_default_exception exception thrown" );
    }
}

class bad_corral_foo : public bad_corral {};

class foo {};

bool is_foo_closed = false;

namespace crrl {
template<>
struct corral_config< foo >
{
    typedef int value_t;
    static bool validator( const value_t & f ) { return f >= 0; }
    static void on_reset( value_t & f )
    {
        Good( "foo has been closed" );
        is_foo_closed = true;
    }
    typedef bad_corral_foo Texception;
};
}   // namespace crrl

void indirect_type_example()
{
    is_foo_closed = false;
    try
    {
        corral<foo> f( 1 );
        Verify( f.get() == 1, "Did indirect_type_example return 1?" );
        Good( "indirect_type_example didn't throw" );

        corral<foo> f_moved( f );    // Check can be moved with indirect type
    }
    catch( bad_corral_foo & )
    {
        Bad( "indirect_type_example threw bad_corral_foo" );
    }
    catch( bad_corral & )
    {
        Bad( "indirect_type_example shouldn't throw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown indirect_type_example exception thrown" );
    }
    Verify( is_foo_closed, "Did indirect_type_example corral_config<foo>::on_reset() get called?" );
}

void indirect_type_bad_value_example()
{
    is_foo_closed = false;
    try
    {
        corral<foo> f( -1 );
        int i = f.get();
        Bad( "indirect_type_bad_value_example threw" );
    }
    catch( bad_corral_foo & )
    {
        Good( "indirect_type_bad_value_example threw bad_corral_foo" );
    }
    catch( bad_corral & )
    {
        Bad( "indirect_type_bad_value_example shouldn't throw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown indirect_type_bad_value_example exception thrown" );
    }
    Verify( ! is_foo_closed, "Check indirect_type_bad_value_example corral...::on_reset() not called?" );
}

class bad_corral_whandle : public bad_corral {};

template< typename Tvalue >
class whandle {};

namespace crrl {
template<typename Tvalue>
struct corral_config< whandle<Tvalue> >
{
    typedef Tvalue value_t;
    static bool validator( const value_t & f ) { return f >= 0; }
    static void on_reset( value_t & f )
    {
        Good( "whandle<T> has been closed" );
        is_foo_closed = true;
    }
    typedef bad_corral_whandle Texception;
};
}   // namespace crrl

void double_indirect_type_example()
{
    is_foo_closed = false;
    try
    {
        corral<whandle<int> > f( 1 );
        Verify( f.get() == 1, "Did double_indirect_type_example return 1?" );
        Good( "double_indirect_type_example didn't throw" );
    }
    catch( bad_corral_whandle & )
    {
        Bad( "double_indirect_type_example threw bad_corral_foo" );
    }
    catch( bad_corral & )
    {
        Bad( "double_indirect_type_example shouldn't throw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown double_indirect_type_example exception thrown" );
    }
    Verify( is_foo_closed, "Did double_indirect_type_example corral...<whandle<T>>::on_reset() get called?" );
}

class bad_corral_custom_whandle : public bad_corral_whandle {};

void double_indirect_type_bad_value_example()
{
    is_foo_closed = false;
    try
    {
        corral<whandle<int>, bad_corral_custom_whandle> f( -11 );
        Verify( f.get() == 1, "Did double_indirect_type_bad_value_example return 1?" );
        Bad( "double_indirect_type_bad_value_example threw" );
    }
    catch( bad_corral_custom_whandle & )
    {
        Good( "double_indirect_type_bad_value_example threw bad_corral_custom_whandle" );
    }
    catch( bad_corral_whandle & )
    {
        Bad( "double_indirect_type_bad_value_example shouldn't threw bad_corral_whandle" );
    }
    catch( bad_corral & )
    {
        Bad( "double_indirect_type_bad_value_example shouldn't throw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown double_indirect_type_bad_value_example exception thrown" );
    }
    Verify( ! is_foo_closed, "Did double_indirect_type_bad_value_example corral...<whandle<T>>::on_reset() get called?" );
}

class bad_outer_corral_foo : public bad_corral {};
class bad_inner_corral_foo : public bad_corral {};

void take_example()
{
    is_foo_closed = false;
    try
    {
        corral<foo, bad_outer_corral_foo> outer( 1 );
        Verify( outer.get() == 1, "Did take_example outer return 1?" );
        Verify( outer.is_valid(), "Is take_example outer valid?" );

        try
        {
            corral<foo, bad_inner_corral_foo> inner( 2 );
            Verify( inner.get() == 2, "Did take_example inner return 2?" );

            inner.take( outer );
            Verify( is_foo_closed, "Did take_example inner corral_config<foo>::on_reset() get called?" );
            Verify( inner.get() == 1, "Did take_example inner return 1?" );
            Verify( ! outer.is_valid(), "Is take_example outer still valid?" );
        }
        catch( bad_corral & )
        {
            Bad( "take_example inner shouldn't throw bad_corral" );
        }

        Good( "take_example outer didn't throw" );

        is_foo_closed = false;

        outer.check();  // Should throw
    }
    catch( bad_inner_corral_foo & )
    {
        Bad( "take_example outer threw bad_inner_corral_foo" );
    }
    catch( bad_outer_corral_foo & )
    {
        Good( "take_example outer threw bad_outer_corral_foo" );
    }
    catch( bad_corral & )
    {
        Bad( "take_example outer shouldn't throw bad_corral" );
    }
    catch( ... )
    {
        Bad( "Unknown take_example outer exception thrown" );
    }
    Verify( ! is_foo_closed, "Did take_example outer corral_config<foo>::on_reset() get called?" );
}

int main( int argc, char * argv[] )
{
    simple_no_value_set_example();
    simple_value_set_example();
    const_simple_value_set_example();
    alternate_exception_example();
    validated_non_throw_example();
    validated_throw_example();
    file_example_1();
    file_example_2();
    file_example_3();
    file_default_exception();
    indirect_type_example();
    indirect_type_bad_value_example();
    double_indirect_type_example();
    double_indirect_type_bad_value_example();
    take_example();

    report();

    return 0;
}
