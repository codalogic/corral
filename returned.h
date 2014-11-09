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

// Note: return_from_function:
// Use the Colvin/Gibbons (aka auto_ptr / auto_ptr_ref shuffle) to transfer
// returned<T> from functions. See last part of:
// http://ptgmedia.pearsoncmg.com/images/020163371X/autoptrupdate/auto_ptr_update.html
// Additional background at: http://www.gotw.ca/gotw/025.htm
// And auto_ptr code in <memory> .

#ifndef RETURNED_H
#define RETURNED_H

#include <exception>

namespace ret {

class bad_returned : public std::exception
{
	virtual const char* what() const throw()
	{
		return "bad_returned exception";
	}
};

template< typename T >
class bad_returned_release : public std::exception
{
	virtual const char* what() const throw()
	{
		return "bad_returned_release exception";
	}
};

template< typename Tvalue >
struct returned_config
{
	// Empty so that without template specialisation code won't compile

	// typedef Tvalue value_t;
	// static bool validator( const Tvalue & ) { return true; }
	// static void on_reset( Tvalue & value ) {}
	// typedef bad_returned Texception;
};

// A simple non-clean-up config.  For example, use as:
// namespace ret {
// template<>
// struct returned_config<int> : public returned_config_default<int> {};
// }
template< typename Tvalue >
struct returned_config_simple
{
	typedef Tvalue value_t;
	static bool validator( const Tvalue & ) { return true; }
	static void on_reset( Tvalue & value ) {}
	typedef bad_returned Texception;
};

template< typename Tvalue, typename Tconfig >
class returned_bridge	// See return_from_function. 1 - define a bridge
{
private:	// returned_bridge is an implementation detail of returned
	template< typename Uvalue, typename Uexception, typename Uconfig > friend class returned;

	explicit returned_bridge( Tvalue value, bool is_valid )
		: m_value( value ), m_is_valid( is_valid )
	{}

	bool m_is_valid;
	Tvalue m_value;
};

template< typename Tvalue,
			typename Texception = typename returned_config<Tvalue>::Texception,
			typename Tconfig = returned_config< Tvalue > >
class returned
{
public:
	typedef typename returned_config<Tvalue>::value_t value_t;
	typedef bool (*validator_t)( const value_t & );

private:
	bool m_is_valid;
	bool m_is_owned;
	value_t m_value;

public:
	returned() : m_is_valid( false ), m_is_owned( false )
	{}
	returned( value_t value ) : m_value( value )
	{
		m_is_valid = m_is_owned = Tconfig::validator( value );
	}
	returned( validator_t validator, value_t value ) : m_value( value )
	{
		m_is_valid = m_is_owned = validator( value );
	}
	template< typename Uvalue, typename Uexception, typename Uconfig > friend class returned;
	template< typename Uexception >
	explicit returned( returned< Tvalue, Uexception, Tconfig > & rhs )
	{
		// Really a move()!
		m_is_valid = m_is_owned = rhs.is_valid();
		if( m_is_valid )
			m_value = rhs.m_value;
		rhs.m_is_valid = rhs.m_is_owned = false;
	}
	operator returned_bridge<Tvalue, Tconfig>()	// See return_from_function. 2 - Cast to create a bridge
	{
		returned_bridge<Tvalue, Tconfig> bridge( m_value, m_is_valid );
		m_is_valid = m_is_owned = false;
		return bridge;
	}
	returned( returned_bridge<Tvalue, Tconfig> bridge )	// See return_from_function. 3 - Construct from bridge
	{
		m_value = bridge.m_value;
		m_is_owned = m_is_valid = bridge.m_is_valid;
	}
	virtual ~returned()
	{
		reset();
	}
	bool is_valid() const { return m_is_valid; }
	void check() const
	{
		if( ! m_is_valid || ! m_is_owned )
			throw Texception();
	}
	value_t & get()
	{
		if( ! m_is_valid || ! m_is_owned )
			throw Texception();
		return m_value;
	}
	const value_t & get() const
	{
		if( ! m_is_valid || ! m_is_owned )
			throw Texception();
		return m_value;
	}
	template< typename Uexception >
	void take( returned< Tvalue, Uexception, Tconfig > & rhs )
	{
		reset();
		if( rhs.m_is_owned && rhs.m_is_valid )
		{
			m_value = rhs.release();
			m_is_owned = m_is_valid = true;
		}
	}
	value_t & release()
	{
		if( ! m_is_valid )
			throw bad_returned_release< Texception >();
		m_is_owned = false;
		return m_value;
	}
	void reset()
	{
		if( m_is_owned && m_is_valid )
			if( ! on_reset( m_value ) )
				Tconfig::on_reset( m_value );
		m_is_valid = m_is_owned = false;
	}

private:
	template< typename Uexception >	// Disable copy assignment
		returned & operator = ( returned< Tvalue, Uexception, Tconfig > & rhs );
	virtual bool /* is_resource_released */ on_reset( value_t & value ) { return false; }
};

} // namespace ret

#endif	// RETURNED_H
