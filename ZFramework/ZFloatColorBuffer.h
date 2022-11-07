#pragma once

#include "ZBuffer.h"
#include "ZStdTypes.h"
#include "ZStdDebug.h"
#include <string>
#include <mutex>

class ZFloatColorBuffer
{
public:
    ZFloatColorBuffer();
	~ZFloatColorBuffer();

    bool        From(ZBuffer* pSrc);
    bool        Init(int64_t nWidth, int64_t nHeight);
	bool        Shutdown();
										
    void        GetPixel(int64_t x, int64_t y, ZFColor& fCol);
	void        SetPixel(int64_t x, int64_t y, ZFColor fCol);

    void        MultRect(ZRect& rArea, double fMult);
    void        AddRect(ZRect& rArea, ZFColor fCol);

    ZBuffer&    GetBuffer() { return mBuffer; }

    static      uint32_t    FloatColToCol(ZFColor fCol);
    static      ZFColor     ColToFloatCol(uint32_t nCol);

protected:
    ZBuffer     mBuffer;

    ZFColor*    mpFloatPixels;

};

