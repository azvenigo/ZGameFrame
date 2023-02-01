#pragma once

#include <map>
#include <list>
#include <string>
#include <limits>
#include <vector>

template <class T>
T limit(T val, T min, T max)
{
    if (val < min)
        return min;
    else if (val > max)
        return max;
    return val;
};

template <class T>
class tPoint
{
public:
	tPoint() 
	{ 
		mX = T{}; 
		mY = T{}; 
	}
	tPoint(T x, T y) 
	{ 
		mX = x; 
		mY = y; 
	}
	tPoint(const tPoint& rhs) 
	{ 
		mX = rhs.mX; 
		mY = rhs.mY; 
	}

	void Set(T x, T y) 
	{ 
		mX = x; 
		mY = y; 
	}
	void Offset(T x, T y) 
	{ 
		mX += x; 
		mY += y; 
	};
	void operator+=(tPoint& p) 
	{ 
		mX += p.mX; 
		mY += p.mY; 
	}
	void operator-=(tPoint& p) 
	{ 
		mX -= p.mX; 
		mY -= p.mY; 
	}

	tPoint operator-() const 
	{ 
		return tPoint(-mX, -mY); 
	}
	tPoint operator+(const tPoint& p) const 
	{ 
		return tPoint(mX + p.mX, mY + p.mY); 
	};

	bool operator==(const tPoint& p) const
	{ 
		return (mX == p.mX && mY == p.mY); 
	}
	bool operator!=(const tPoint& p) const
	{ 
		return (mX != p.mX || mY != p.mY); 
	}

	T mX;
	T mY;
};


typedef tPoint<int64_t> ZPoint;
typedef tPoint<double>	ZFPoint;




template<typename T> class tColor 
{
public:
    tColor()
    {
        a = 0.0;
        r = 0.0;
        g = 0.0;
        b = 0.0;
    }

    tColor(T _a, T _r, T _g, T _b)
    {
        a = _a;
        r = _r;
        g = _g;
        b = _b;
    }

    tColor(const tColor& rhs)
    {
        a = rhs.a;
        r = rhs.r;
        g = rhs.g;
        b = rhs.b;
    }

    tColor operator-() const
    {
        return tColor(a, -r, -g, -b);
    }

    tColor operator+(tColor& c) const
    {
        return tColor(a, r + c.r, g + c.g, b + c.b);
    }

    void operator+=(const tColor& c)
    {
        r += c.r;
        g += c.g;
        b += c.b;
    }


    tColor operator-(tColor& c) const
    {
        return tColor(a, r - c.r, g - c.g, b - c.b);
    }

    void operator-=(const tColor& c)
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
    }

    tColor operator*(T mult) const
    {
        return tColor(a, r * mult, g * mult, b * mult);
    }

    void operator*=(T mult)
    {
        r *= mult;
        g *= mult;
        b *= mult;
    }


    T a;
    T r;
    T g;
    T b;
};

typedef tColor<double> ZFColor;




template<typename T> class tColorVertex : public tPoint<T>
{
public:
    tColorVertex() : tPoint<T>()
	{ 
		mColor = 0; 
	}
    tColorVertex(T x, T y, uint32_t col) : tPoint<T>(x, y)
	{ 
		mColor = col; 
	}
    tColorVertex(const tColorVertex& rhs) : tPoint<T>(rhs.mX,rhs.mY)
	{ 
		mColor = rhs.mColor; 
	}
    tColorVertex operator-() const
	{ 
		return tColorVertex(-tPoint<T>::mX, -tPoint<T>::mY, mColor);
	}
    tColorVertex operator+(tColorVertex& v) const
	{ 
		return tColorVertex(tPoint<T>::mX + v.mX, tPoint<T>::mY + v.mY, mColor);
	}

	uint32_t mColor;
};


template<typename T> class tUVVertex : public tPoint<T>
{
public:
    tUVVertex() : tPoint<T>()
    {
        mU = 0;
        mV = 0;
    }
    tUVVertex(T x, T y, T u, T v) : tPoint<T>(x, y)
    {
        mU = u;
        mV = v;
    }
    tUVVertex(const tUVVertex& rhs) : tPoint<T>(rhs.mX, rhs.mY)
    {
        mU = rhs.mU;
        mV = rhs.mV;
    }
    tUVVertex operator-() const
    {
        return tUVVertex(-tPoint<T>::mX, -tPoint<T>::mY, mU, mV);
    }
    tUVVertex operator+(tUVVertex& v) const
    {
        return tUVVertex(tPoint<T>::mX + v.mX, tPoint<T>::mY + v.mY, mU, mV);
    }

    T  mU;
    T  mV;
};

typedef tUVVertex<double> ZUVVertex;
typedef tColorVertex<double> ZColorVertex;

typedef std::vector<ZUVVertex> tUVVertexArray;
typedef std::vector<ZColorVertex> tColorVertexArray;




template<typename T>
class tRect
{
public:
    // Constructors
	tRect()
	{
		left = T{};
		top = T{};
		right = T{};
		bottom = T{};

	}
	tRect(T l, T t, T r, T b) 
	{ 
		left = l; 
		top = t; 
		right = r; 
		bottom = b; 
	}
	tRect(T nWidth, T nHeight) 
	{ 
		left = 0; 
		top = 0; 
		right = nWidth; 
		bottom = nHeight; 
	}

	tRect(const tRect& r) 
	{ 
		left = r.left; 
		top = r.top; 
		right = r.right; 
		bottom = r.bottom; 
	}

	tRect(tRect* const pRect) 
	{ 
		left = pRect->left; 
		top = pRect->top; 
		right = pRect->right; 
		bottom = pRect->bottom; 
	}

	T	Width() const 
	{ 
		return right - left; 
	}
	T	Height() const 
	{ 
		return bottom - top; 
	}
	
    // Queries
    bool PtInRect(tPoint<T>& p) const
	{
		return (p.mX >= left && p.mY >= top && p.mX < right&& p.mY < bottom);
	}
    bool PtInRect(T x, T y) const
	{
		return (x >= left && y >= top && x < right&& y < bottom);
	}
    bool Overlaps(const tRect& pRect) const
	{
		return !((left >= pRect.right) || 
			     (right <= pRect.left) || 
			     (top >= pRect.bottom) || 
			     (bottom <= pRect.top));
	}

    // Operations
    void SetRect(T l, T t, T r, T b)
	{
		left = l;
		top = t;
		right = r;
		bottom = b;
	}
    void SetRect(const tRect& r)
	{
		left = r.left;
		top = r.top;
		right = r.right;
		bottom = r.bottom;
	}

    void InflateRect(T x, T y)
	{
		left -= x;
		right += x;
		top -= y;
		bottom += y;
	}
    void DeflateRect(T x, T y)
	{
		left += x;
		right -= x;
		top += y;
		bottom -= y;
	}

    void MoveRect(T x, T y)
	{
		int64_t nOffsetX = (x - left);
		int64_t nOffsetY = (y - top);

		left += nOffsetX;
		right += nOffsetX;
		top += nOffsetY;
		bottom += nOffsetY;
	}
    void MoveRect(ZPoint& p)
	{
		int64_t nOffsetX = (p.mX - left);
		int64_t nOffsetY = (p.mY - top);

		left += nOffsetX;
		right += nOffsetX;
		top += nOffsetY;
		bottom += nOffsetY;
	}
    void OffsetRect(T x, T y)
	{
		left += x;
		right += x;
		top += y;
		bottom += y;
	}
    void OffsetRect(ZPoint& p)
	{
		OffsetRect(p.mX, p.mY);
	}
    void NormalizeRect()
	{
		int64_t nTemp;

		if (left > right)
		{
			nTemp = left;
			left = right;
			right = nTemp;
		}

		if (top > bottom)
		{
			nTemp = top;
			top = bottom;
			bottom = nTemp;
		}
	}

    // Operations that fill '*this' with result
    bool IntersectRect(const tRect& rhs)
	{
		if (left < rhs.left)
			left = rhs.left;

		if (top < rhs.top)
			top = rhs.top;

		if (right > rhs.right)
			right = rhs.right;

		if (bottom > rhs.bottom)
			bottom = rhs.bottom;

		if (left > right || top > bottom)
		{
			left = 0;
			top = 0;
			right = 0;
			bottom = 0;
			return false;
		}

		return true;
	}
    bool UnionRect(const tRect& rhs)
	{
		if (left > rhs.left)
			left = rhs.left;

		if (top > rhs.top)
			top = rhs.top;

		if (right < rhs.right)
			right = rhs.right;

		if (bottom < rhs.bottom)
			bottom = rhs.bottom;

		return true;
	}

    tRect CenterInRect(const tRect& outerRect)
    {
        T tempX = outerRect.left + ((outerRect.Width() - Width()) / 2);
        T tempY = outerRect.top + ((outerRect.Height() - Height()) / 2);
        return tRect(tempX, tempY, tempX + Width(), tempY + Height());
    }


    // Additional Operations
    void operator=(const tRect& r)
	{
		left = r.left;
		top = r.top;
		right = r.right;
		bottom = r.bottom;
	}

    void operator+=(tPoint<T>& p)
	{
		OffsetRect(p.mX, p.mY);
	}
    void operator-=(tPoint<T>& p)
	{
		OffsetRect(-p.mX, -p.mY);
	}
    void operator&=(const tRect& r)
	{
		IntersectRect(&r);
	}
    void operator|=(const tRect& r)
	{
		UnionRect(&r);
	}


	bool operator==(const tRect& r)	
	{ 
		return ((left == r.left) && (top == r.top) && (right == r.right) && (bottom == r.bottom));
	}
	bool operator!=( const tRect& r)	
	{ 
		return (left != r.left)||(top != r.top)||(right != r.right)||(bottom != r.bottom);	
	}
public:
	T	left;
	T	top;
	T	right;
	T	bottom;
};

typedef tRect<int64_t> ZRect;
typedef tRect<double>  cFRect;

typedef std::list<ZRect> tRectList;

/////////////////////////////////////////////////////////////////////////////


// Standard maps
typedef std::map<std::string, std::string> tKeyValueMap;





#ifndef _WIN32

/////////////////////////////////////////////////////////////////////////////
// code readability macros
#define MAXINT32 std::numeric_limits<int32_t>::max()
#define MAXINT64 std::numeric_limits<int64_t>::max()

#define MININT32 std::numeric_limits<int32_t>::min()
#define MININT64 std::numeric_limits<int64_t>::min()


#endif