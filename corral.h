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
// corral<T> from functions. See last part of:
// http://ptgmedia.pearsoncmg.com/images/020163371X/autoptrupdate/auto_ptr_update.html
// Additional background at: http://www.gotw.ca/gotw/025.htm
// And auto_ptr code in <memory> .

#ifndef CORRAL_H
#define CORRAL_H

#include <exception>

namespace crrl {    // 'corral' without the vowels!

class bad_corral : public std::exception
{
    virtual const char * what() const throw()
    {
        return "bad_corral exception";
    }
};

template< typename T >
class bad_corral_release : public std::exception
{
    virtual const char * what() const throw()
    {
        return "bad_corral_release exception";
    }
};

template< typename Tvalue >
struct corral_config
{
    // Empty so that without template specialisation code won't compile

    // typedef Tvalue value_t;
    // static bool validator( const Tvalue & ) { return true; }
    // static void on_reset( Tvalue & value ) {}
    // typedef bad_corral Texception;
};

// A simple non-clean-up config.  For example, use as:
// namespace corral {
// template<>
// struct corral_config<int> : public corral_config_default<int> {};
// }
template< typename Tvalue >
struct corral_config_simple
{
    typedef Tvalue value_t;
    static bool validator( const Tvalue & ) { return true; }
    static void on_reset( Tvalue & value ) {}
    typedef bad_corral Texception;
};

template< typename Tvalue, typename Tconfig >
class corral_bridge   // See return_from_function. 1 - define a bridge
{
private:    // corral_bridge is an implementation detail of corral
    template< typename Uvalue, typename Uexception, typename Uconfig > friend class corral;

    explicit corral_bridge( Tvalue value, bool is_valid )
        : m_value( value ), m_is_valid( is_valid )
    {}

    bool m_is_valid;
    Tvalue m_value;
};

template< typename Tvalue,
            typename Texception = typename corral_config<Tvalue>::Texception,
            typename Tconfig = corral_config< Tvalue > >
class corral
{
public:
    typedef typename corral_config<Tvalue>::value_t value_t;
    typedef bool (*validator_t)( const value_t & );

private:
    bool m_is_valid;
    bool m_is_owned;
    value_t m_value;

public:
    corral() : m_is_valid( false ), m_is_owned( false )
    {}
    corral( value_t value ) : m_value( value )
    {
        m_is_valid = m_is_owned = Tconfig::validator( value );
    }
    corral( value_t value, validator_t validator ) : m_value( value )
    {
        m_is_valid = m_is_owned = validator( value );
    }
    template< typename Uvalue, typename Uexception, typename Uconfig > friend class corral;
    template< typename Uexception >
    explicit corral( corral< Tvalue, Uexception, Tconfig > & rhs )
    {
        // Really a move()!
        m_is_valid = m_is_owned = rhs.is_valid();
        if( m_is_valid )
            m_value = rhs.m_value;
        rhs.m_is_valid = rhs.m_is_owned = false;
    }
    operator corral_bridge<Tvalue, Tconfig>() // See return_from_function. 2 - Cast to create a bridge
    {
        corral_bridge<Tvalue, Tconfig> bridge( m_value, m_is_valid );
        m_is_valid = m_is_owned = false;
        return bridge;
    }
    corral( corral_bridge<Tvalue, Tconfig> bridge ) // See return_from_function. 3 - Construct from bridge
    {
        m_value = bridge.m_value;
        m_is_owned = m_is_valid = bridge.m_is_valid;
    }
    virtual ~corral()
    {
        reset();
    }
    bool is_valid() const { return m_is_owned && m_is_valid; }
    void check() const
    {
        if( ! is_valid() )
            throw Texception();
    }
    value_t & get()
    {
        if( ! is_valid() )
            throw Texception();
        return m_value;
    }
    const value_t & get() const
    {
        if( ! is_valid() )
            throw Texception();
        return m_value;
    }
    template< typename Uexception >
    void take( corral< Tvalue, Uexception, Tconfig > & rhs )
    {
        reset();
        if( rhs.is_valid() )
        {
            m_value = rhs.release();
            m_is_owned = m_is_valid = true;
        }
    }
    value_t & release()
    {
        if( ! is_valid() )
            throw bad_corral_release< Texception >();
        m_is_owned = false;
        return m_value;
    }
    void reset()
    {
        if( is_valid() )
            if( ! on_reset( m_value ) )
                Tconfig::on_reset( m_value );
        m_is_valid = m_is_owned = false;
    }

private:
    template< typename Uexception > // Disable copy assignment
        corral & operator = ( const corral< Tvalue, Uexception, Tconfig > & rhs );
    virtual bool /* is_resource_released */ on_reset( value_t & value ) { return false; }
};

} // namespace crrl

#endif  // CORRAL_H
