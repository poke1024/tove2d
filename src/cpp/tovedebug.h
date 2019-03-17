/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#if TOVE_DEBUG

#include <iostream>
#include <iomanip>

BEGIN_TOVE_NAMESPACE

namespace debug {

class save_ios_fmt {
	std::ostream &os;
	std::ios state;
public:
	save_ios_fmt(std::ostream &os) : os(os), state(nullptr) {
		state.copyfmt(os);
	}

	~save_ios_fmt() {
		os.copyfmt(state);
	}
};

// indent_output was taken from
// https://stackoverflow.com/questions/9599807/how-to-add-indention-to-the-stream-operator
class indent_output : public std::streambuf
{
    std::streambuf*     myDest;
    bool                myIsAtStartOfLine;
    std::string         myIndent;
    std::ostream*       myOwner;
protected:
    virtual int         overflow( int ch )
    {
        if ( myIsAtStartOfLine && ch != '\n' ) {
            myDest->sputn( myIndent.data(), myIndent.size() );
        }
        myIsAtStartOfLine = ch == '\n';
        return myDest->sputc( ch );
    }
public:
    explicit            indent_output( 
                            std::streambuf* dest, int indent = 4 )
        : myDest( dest )
        , myIsAtStartOfLine( true )
        , myIndent( indent, ' ' )
        , myOwner( NULL )
    {
    }
    explicit            indent_output(
                            std::ostream& dest, int indent = 4 )
        : myDest( dest.rdbuf() )
        , myIsAtStartOfLine( true )
        , myIndent( indent, ' ' )
        , myOwner( &dest )
    {
        myOwner->rdbuf( this );
    }
    virtual             ~indent_output()
    {
        if ( myOwner != NULL ) {
            myOwner->rdbuf( myDest );
        }
    }
};

class color {
	const uint32_t m_color;

public:
	color(uint32_t color) : m_color(color) {
	}

	friend std::ostream &operator<<(std::ostream &os, const color &c) {
        const float r = (c.m_color & 0xff) / 255.0;
        const float g = ((c.m_color >> 8) & 0xff) / 255.0;
        const float b = ((c.m_color >> 16) & 0xff) / 255.0;
        const float a = ((c.m_color >> 24) & 0xff) / 255.0;


		tove::debug::save_ios_fmt fmt(os);
		os << std::fixed << std::setprecision(2);

		os << "rgba(" <<
			r << ", " <<
			g << ", " <<
			b << ", " <<
			a << ")";

		return os;
	}
};

template<typename T>
class xform {
	const T * const matrix;

public:
	xform(const T *matrix) : matrix(matrix) {
	}

	friend std::ostream &operator<<(std::ostream &os, const xform &m) {
		tove::debug::save_ios_fmt fmt(os);

		os << std::fixed << std::setprecision(2);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 3; j++) {
				os << std::setw(6);
				os << (j > 0 ? "\t" : "") << m.matrix[3 * i + j];
			}
			os << std::endl;
		}

		return os;
	}
};

} // end namespace debug

END_TOVE_NAMESPACE

#endif // TOVE_DEBUG
