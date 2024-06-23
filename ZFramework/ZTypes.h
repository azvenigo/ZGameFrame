#pragma once

#include <map>
#include <list>
#include <string>
#include <limits>
#include <vector>

// Helper template to ensure value is bounded between min and max
template <class T>
void limit(T& val, T min, T max)
{
    if (val < min)
        val = min;
    else if (val > max)
        val = max;
};

template <typename T>
T truncate(T number, int decimalPlaces) 
{
    T multiplier = (T)pow(10, decimalPlaces);
    T numberfloor = floorf(number * multiplier);
    return numberfloor / multiplier;
}

template <class T>
class tPoint
{
public:
	tPoint() 
	{ 
		x = T{}; 
		y = T{}; 
	}
	tPoint(T _x, T _y) 
	{ 
		x = _x; 
		y = _y; 
	}
	tPoint(const tPoint& rhs) 
	{ 
		x = rhs.x; 
		y = rhs.y; 
	}

	void Set(T _x, T _y) 
	{ 
		x = _x; 
		y = _y; 
	}
	void Offset(T _x, T _y) 
	{ 
		x += _x; 
		y += _y; 
	};
	void operator+=(tPoint& p) 
	{ 
		x += p.x; 
		y += p.y; 
	}
	void operator-=(tPoint& p) 
	{ 
		x -= p.x; 
		y -= p.y; 
	}

	tPoint operator-() const 
	{ 
		return tPoint(-x, -y); 
	}
	tPoint operator+(const tPoint& p) const 
	{ 
		return tPoint(x + p.x, y + p.y); 
	};

	bool operator==(const tPoint& p) const
	{ 
		return (x == p.x && y == p.y); 
	}
	bool operator!=(const tPoint& p) const
	{ 
		return (x != p.x || y != p.y); 
	}

	T x;
	T y;
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
    tColorVertex(const tColorVertex& rhs) : tPoint<T>(rhs.x,rhs.y)
	{ 
		mColor = rhs.mColor; 
	}
    tColorVertex operator-() const
	{ 
		return tColorVertex(-tPoint<T>::x, -tPoint<T>::y, mColor);
	}
    tColorVertex operator+(tColorVertex& v) const
	{ 
		return tColorVertex(tPoint<T>::x + v.x, tPoint<T>::y + v.y, mColor);
	}

	uint32_t mColor;
};


template<typename T> class tUVVertex : public tPoint<T>
{
public:
    tUVVertex() : tPoint<T>()
    {
        u = 0;
        v = 0;
    }
    tUVVertex(T _x, T _y, T _u, T _v) : tPoint<T>(_x, _y)
    {
        u = u;
        v = v;
    }
    tUVVertex(const tUVVertex& rhs) : tPoint<T>(rhs.x, rhs.y)
    {
        u = rhs.u;
        v = rhs.v;
    }
    tUVVertex operator-() const
    {
        return tUVVertex(-tPoint<T>::x, -tPoint<T>::y, u, v);
    }
    tUVVertex operator+(tUVVertex& _v) const
    {
        return tUVVertex(tPoint<T>::x + v.x, tPoint<T>::y + v.y, u, v);
    }

    T  u;
    T  v;
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
		return (p.x >= left && p.y >= top && p.x < right&& p.y < bottom);
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

    tPoint<T> TL() const
    {
        return tPoint<T>(left, top);
    }

    tPoint<T> BR() const
    {
        return tPoint<T>(right, bottom);
    }

    T Area() const
    {
        return (right - left) * (bottom - top);
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
		int64_t nOffsetX = (p.x - left);
		int64_t nOffsetY = (p.y - top);

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
    void OffsetRect(const ZPoint& p)
	{
		OffsetRect(p.x, p.y);
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
        NormalizeRect();

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

    tRect LimitToOuter(const tRect& outerRect)
    {
        // cannot limit if outer rect is smaller
        if (Width() > outerRect.Width() || Height() > outerRect.Height())
            return *this;

        if (left < outerRect.left)
        {
            T xoffset = outerRect.left - left;
            left += xoffset;
            right += xoffset;
        }
        else if (right > outerRect.right)
        {
            T xoffset = right - outerRect.right;
            right -= xoffset;
            left -= xoffset;
        }

        if (top < outerRect.top)
        {
            T yoffset = outerRect.top - top;
            top += yoffset;
            bottom += yoffset;
        }
        else if (bottom > outerRect.bottom)
        {
            T yoffset = bottom - outerRect.bottom;
            top -= yoffset;
            bottom -= yoffset;
        }           

        return *this;
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
		OffsetRect(p.x, p.y);
	}
    void operator-=(tPoint<T>& p)
	{
		OffsetRect(-p.x, -p.y);
	}
    void operator&=(const tRect& r)
	{
		IntersectRect(&r);
	}
    void operator|=(const tRect& r)
	{
		UnionRect(&r);
	}


	bool operator==(const tRect& r) const
	{ 
		return ((left == r.left) && (top == r.top) && (right == r.right) && (bottom == r.bottom));
	}
	bool operator!=( const tRect& r) const
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


// Standard containers
typedef std::map<std::string, std::string> tKeyValueMap;
typedef std::list<std::string> tStringList;





#ifndef _WIN32

/////////////////////////////////////////////////////////////////////////////
// code readability macros
#define MAXINT32 std::numeric_limits<int32_t>::max()
#define MAXINT64 std::numeric_limits<int64_t>::max()

#define MININT32 std::numeric_limits<int32_t>::min()
#define MININT64 std::numeric_limits<int64_t>::min()


#endif