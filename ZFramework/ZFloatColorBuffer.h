#pragma once

#include "ZBuffer.h"
#include "ZTypes.h"
#include "ZDebug.h"
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

    tZBufferPtr    GetBuffer() { return mpBuffer; }

    static      uint32_t    FloatColToCol(ZFColor fCol);
    static      ZFColor     ColToFloatCol(uint32_t nCol);

protected:
    tZBufferPtr     mpBuffer;

    ZFColor*    mpFloatPixels;

};

