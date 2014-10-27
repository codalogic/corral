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

#include "returned.h"

#include "annotate-lite.h"

#include <cstdio>

#if defined(_MSC_VER)
// Require error when nonstandard extension used :
//		'token' : conversion from 'type' to 'type'
//		e.g. reject foo( /*no const*/ foo & ) copy constructor
#pragma warning(error: 4239)
#endif

using namespace ret;

namespace ret {
template<>
struct returned_config<int> : public returned_config_simple<int> {};
}

void simple_no_value_set_example()
{
	try
	{
		returned<int> r;
		int t = r.get();
		Bad( "simple_no_value_set_example didn't throw" );
	}
	catch( bad_returned & )
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
		returned<int> r( 1 );
		int t = r.get();
		Good( "simple_value_set_example didn't throw" );
	}
	catch( bad_returned & )
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
		const returned<int> r( 1 );
		int t = r.get();
		Good( "const_simple_value_set_example didn't throw" );
	}
	catch( bad_returned & )
	{
		Bad( "const_simple_value_set_example threw" );
	}
	catch( ... )
	{
		Bad( "const_simple_value_set_example threw wrong exception" );
	}
}

class bad_alternate_returned : public bad_returned {};

returned<int> my_op()
{
	return returned<int>();
}

void alternate_exception_example()
{
	try
	{
		returned<int, bad_alternate_returned> r( my_op() );
		int t = r.get();
		Bad( "alternate_exception_example didn't throw" );
	}
	catch( bad_alternate_returned & )
	{
		Good( "alternate_exception_example threw bad_alternate_returned" );
	}
	catch( bad_returned & )
	{
		Bad( "alternate_exception_example threw bad_returned" );
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

returned<int> my_validated_op( int v )
{
	return returned<int>( not_zero, v );
}

void validated_non_throw_example()
{
	try
	{
		returned<int> r( my_validated_op( 1 ) );
		int t = r.get();
		Good( "validated_non_throw_example didn't throw" );
	}
	catch( bad_returned & )
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
		returned<int> r( my_validated_op( 0 ) );
		int t = r.get();
		Bad( "validated_throw_example didn't throw" );
	}
	catch( bad_returned & )
	{
		Good( "validated_throw_example threw" );
	}
	catch( ... )
	{
		Bad( "validated_throw_example threw wrong exception" );
	}
}

class bad_returned_file : public bad_returned {};

namespace ret {
template<>
struct returned_config< FILE * > : public returned_config_simple< FILE * >
{
	typedef FILE * value_t;
	static bool validator( const value_t & f ) { return f != 0; }
	static void on_reset( value_t & f )
	{
		Good( "file_returned on_reset called" );
		if( f )
			fclose( f );
	}
	typedef bad_returned_file Texception;
};
}	// namespace ret

class bad_file_in_1 : public bad_returned_file {};
class bad_file_in_2 : public bad_returned_file {};

void file_example_1()
{
	try
	{
		returned<FILE *, bad_file_in_1> fin1_1( fopen( "test-exists.txt", "r" ) );
		fin1_1.check();
		returned<FILE *, bad_file_in_2> fin1_2( fopen( "test-not-exists.txt", "r" ) );
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
	catch( bad_returned & )
	{
		Bad( "file_example_1 threw bad_returned" );
	}
	catch( ... )
	{
		Bad( "Unknown file_example_1 excpetion thrown" );
	}
}

returned<FILE *> open_file( const char * name, const char * mode )
{
	return returned<FILE *>( fopen( name, mode ) );
}

void file_example_2()
{
	try
	{
		returned<FILE *, bad_file_in_1> fin2_1( open_file( "test-exists.txt", "r" ) );
		fin2_1.check();
		returned<FILE *, bad_file_in_2> fin2_2( open_file( "test-not-exists.txt", "r" ) );
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
	catch( bad_returned & )
	{
		Bad( "file_example_1 threw bad_returned" );
	}
	catch( ... )
	{
		Bad( "Unknown file_example_1 excpetion thrown" );
	}
}

void file_example_3()
{
	try
	{
		returned<FILE *> fin3_1( open_file( "test-in-1.txt", "r" ) );
		if( fin3_1.is_valid() )
		{
			returned<FILE *> fin3_2 = open_file( "test-in-2.txt", "r" );
			if( fin3_2.is_valid() )
			{
			}
		}
		Good( "file_example_2 didn't throw" );
	}
	catch( bad_returned & )
	{
		Bad( "file_example_2 shouldn't throw" );
	}
	catch( ... )
	{
		Bad( "Unknown file_example_2 excpetion thrown" );
	}
}

void file_default_exception()
{
	try
	{
		returned<FILE *> fin3_1( open_file( "test-in-1.txt", "r" ) );
		fin3_1.check();
		Bad( "file_default_exception didn't throw" );
	}
	catch( bad_returned_file & )
	{
		Good( "file_default_exception threw bad_returned_file" );
	}
	catch( bad_returned & )
	{
		Bad( "file_default_exception shouldn't throw bad_returned" );
	}
	catch( ... )
	{
		Bad( "Unknown file_default_exception excpetion thrown" );
	}
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

	report();

	return 0;
}
