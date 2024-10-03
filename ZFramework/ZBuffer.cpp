#include "ZBuffer.h"
#include <algorithm>
#include <filesystem>
#include "ZMemBuffer.h"
#include "ZColor.h"
#include "helpers/StringHelpers.h"
#include "ZRasterizer.h"
#include <math.h>
#include <fstream>
#include "easyexif/exif.h"
#include "lunasvg.h"
#include "mio/mmap.hpp"
#include <intrin.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/*#ifdef _WIN64
#include <GdiPlus.h>
using namespace Gdiplus;
#include "GDIImageTags.h"
#endif*/

using namespace std;

#include "ZGraphicSystem.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ZBuffer::ZBuffer()
{
	mpPixels                = NULL;
    mRenderState            = kFreeToModify;
    mbHasAlphaPixels = false;
	mSurfaceArea.SetRect(0,0,0,0);
}

ZBuffer::ZBuffer(ZBuffer* pSrc) : ZBuffer()
{
    ZRect rArea(pSrc->mSurfaceArea);
    Init(rArea.Width(), rArea.Height());
    CopyPixels(pSrc);
}


ZBuffer::~ZBuffer()
{
	Shutdown();
}

bool ZBuffer::Init(int64_t nWidth, int64_t nHeight)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);


	ZASSERT(nWidth > 0 && nHeight > 0);
	if (nWidth > 0 && nHeight > 0)
	{
        if (!mpPixels || nWidth * nHeight != mSurfaceArea.Width() * mSurfaceArea.Height())   // if the number of pixels hasn't changed, no need to reallocate
        {
            delete[] mpPixels;
            mpPixels = new uint32_t[nWidth * nHeight];
        }

		mSurfaceArea.SetRect(0, 0, nWidth, nHeight);
//        memset(mpPixels, 0, nWidth * nHeight * sizeof(uint32_t));
        Fill(0);
        mbHasAlphaPixels = false;

		return true;
	}
    else
    {
        delete[] mpPixels;
        mpPixels = nullptr;
    }


	return false;
}

bool ZBuffer::Shutdown()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (mpPixels)
	{
		delete[] mpPixels;
		mpPixels = NULL;
	}

	return true;
};


bool ZBuffer::LoadBuffer(const string& sFilename)
{
    std::filesystem::path filename(sFilename);

    if (!std::filesystem::exists(filename))
    {
        ZDEBUG_OUT("ZBuffer::LoadBuffer failed to load ", sFilename, "..does not exist\n");
        return false;
    }

    string sExt = filename.extension().string();
    SH::makelower(sExt);
    if (sExt == ".svg")
        return LoadFromSVG(sFilename);


#ifdef STB_IMAGE_IMPLEMENTATION
    int width;
    int height;
    int channels;



//    uint32_t nFileSize = (uint32_t)std::filesystem::file_size(sFilename);


//    std::ifstream imageFile(sFilename.c_str(), ios::in | ios::binary);    // open and set pointer at end
//    ZMemBufferPtr imageBuf(new ZMemBuffer(nFileSize));
//    imageFile.read((char*)imageBuf->data(), nFileSize);
//    imageBuf->seekp(nFileSize);


    mio::mmap_source ro_mmap;
    std::error_code error;
    ro_mmap.map(sFilename, error);
    if (error)
    {
        ZDEBUG_OUT("ERROR: Could not open source ", sFilename, "\n");
        return false;
    }

    uint8_t* pScanFileData = (uint8_t*)ro_mmap.data();
    uint32_t nFileSize = (uint32_t)std::filesystem::file_size(sFilename);

    mEXIF.clear();
    if (sExt == ".jpg" || sExt == ".jpeg")
        mEXIF.parseFrom(pScanFileData, nFileSize);
    





//    uint8_t* pImage = stbi_load(sFilename.c_str(), &width, &height, &channels, 4);
    uint8_t* pImage = stbi_load_from_memory(pScanFileData, nFileSize, &width, &height, &channels, 4);
    if (!pImage)
    {
        ZDEBUG_OUT("ZBuffer::LoadBuffer failed to load ", sFilename, "\n");
        return false;
    }

//    cout << "LoadBuffer() About to Shutdown\n";
    Shutdown(); // Clear out any existing data

    if (!Init(width, height))
    {
        stbi_image_free(pImage);
        return false;
    }
//    cout << "LoadBuffer() After init\n";


    uint64_t nAlphaSum = 0;

    uint32_t* pDest = mpPixels;
    for (uint32_t* pSrc = (uint32_t*)pImage; pSrc < (uint32_t*)(pImage + width * height * 4); pSrc++)
    {
        uint32_t col = *pSrc;
        uint32_t a = (col & 0xff000000);
        uint32_t b = (col & 0x00ff0000) >> 16;
        uint32_t r = (col & 0x000000ff) << 16;
        uint32_t g = (col & 0x0000ff00);

        uint32_t newCol = a | r | g | b;

        nAlphaSum += (a >> 24); // add up all alpha values

        *pDest = newCol;
        pDest++;
    }

    // if the sum of all pixel alphas is less than number of pixels * 0xff, then there are alpha pixels
    if (nAlphaSum < width * height * 0xff)
        mbHasAlphaPixels = true;

    stbi_image_free(pImage);

    if (mEXIF.Orientation != 0)
    {
        eOrientation reverse;
        
        // for left and right, the reverse is the opposite rotation. For all others, it's the same operation to reverse
        if ((eOrientation)mEXIF.Orientation == kLeft)
            reverse = kRight;
        else if ((eOrientation)mEXIF.Orientation == kRight)
            reverse = kLeft;
        else
            reverse = (eOrientation)mEXIF.Orientation;


        Rotate(reverse);
    }


    return true;




#else 
	Bitmap bitmap(SH::string2wstring(sFilename).c_str());
    if (bitmap.GetLastStatus() != Status::Ok)
    {
        //OutputDebugImmediate("ZBuffer::LoadBuffer failed to load %s. Status:%d\n", sFilename.c_str(), bitmap.GetLastStatus());
        ZDEBUG_OUT("ZBuffer::LoadBuffer failed to load ", sFilename, " status:", bitmap.GetLastStatus());
        return false;
    }


    uint32_t nPropertySize = 0;
    uint32_t nPropertyCount = 0;

    bitmap.GetPropertySize(&nPropertySize, &nPropertyCount);


    uint32_t nHeight = bitmap.GetHeight();

    PropertyItem* pPropBuffer = (PropertyItem*)malloc(nPropertySize);
    bitmap.GetAllPropertyItems(nPropertySize, nPropertyCount, pPropBuffer);

    ZDEBUG_OUT("Properties for image:", sFilename);
    for (size_t i = 0; i < nPropertyCount; i++)
    {
        PropertyItem* pi = &pPropBuffer[i];
        if (pi->id == PropertyTagOrientation)
        {
            auto orientation = *((uint16_t*)pi->value);
            if (orientation != 1)
            {
                ZDEBUG_OUT("Rotating image. Orientation property:", orientation);

                switch (orientation)
                {
                case 2:
                    bitmap.RotateFlip(RotateNoneFlipX);
                    break;
                case 3:
                    bitmap.RotateFlip(Rotate180FlipNone);
                    break;
                case 4:
                    bitmap.RotateFlip(Rotate180FlipX);
                    break;
                case 5:
                    bitmap.RotateFlip(Rotate90FlipX);
                    break;
                case 6:
                    bitmap.RotateFlip(Rotate90FlipNone);
                    break;
                case 7:
                    bitmap.RotateFlip(Rotate270FlipX);
                    break;
                case 8:
                    bitmap.RotateFlip(Rotate270FlipNone);
                    break;
                }
            }
        }

        BufferProp prop;
        prop.sName = GDIHelpers::TagFromID(pi->id);
        prop.sType = GDIHelpers::TypeString(pi->type);
        int32_t nLength = pi->length;
        if (nLength > 128)
            nLength = 128;
        prop.sValue = GDIHelpers::ValueStringByType(pi->type, pi->value, nLength);
        mBufferProps.push_back(prop);

//        OutputDebugImmediate("%d:%s\n", i, GDIHelpers::PropertyItemToString(&pPropBuffer[i]).c_str());
        ZDEBUG_OUT_LOCKLESS(i, ":", GDIHelpers::PropertyItemToString(&pPropBuffer[i]));
    }
    

    free(pPropBuffer);

	Shutdown(); // Clear out any existing data

	if (!Init(bitmap.GetWidth(), bitmap.GetHeight()))
		return false;

	for (int64_t y = 0; y < (int64_t) bitmap.GetHeight(); y++)
	{
		for (int64_t x = 0; x < (int64_t) bitmap.GetWidth(); x++)
		{
			Color col = 0;
			bitmap.GetPixel((INT)x, (INT)y, &col);
			SetPixel(x, y, col.GetValue());
		}
	}

	return true;
#endif
}

bool ZBuffer::LoadFromSVG(const std::string& sName)
{
    auto doc = lunasvg::Document::loadFromFile(sName);
    //doc->get
    auto svgbitmap = doc->renderToBitmap((uint32_t)mSurfaceArea.Width(), (uint32_t)mSurfaceArea.Height()); // if the buffer is already initialized, this will render at the same dimensions. If not these will be 0x0 & it will render at the SVG embedded dimensions

    int64_t w = (int64_t)svgbitmap.width();
    int64_t h = (int64_t)svgbitmap.height();
    uint32_t s = svgbitmap.stride();

    if (!Init(w, h))
        return false;

    for (int64_t y = 0; y < h; y++)
    {
        memcpy(mpPixels + y * w, svgbitmap.data() + y * s, w * 4);
    }

    mbHasAlphaPixels = true;

    return true;
}



bool ZBuffer::SaveBuffer(const std::string& sName)
{
    std::filesystem::path filename(sName);
    string sExt = filename.extension().string();
    SH::makelower(sExt);

    int64_t w = (int64_t)mSurfaceArea.Width();
    int64_t h = (int64_t)mSurfaceArea.Height();

    uint32_t* pSwizzled = new uint32_t[w * h];

    uint32_t* pDest = pSwizzled;
    for (uint32_t* pSrc = mpPixels; pSrc < (uint32_t*)(mpPixels + (w * h)); pSrc++)
    {
        uint32_t col = *pSrc;
        uint32_t a = (col & 0xff000000);
        uint32_t b = (col & 0x00ff0000) >> 16;
        uint32_t r = (col & 0x000000ff) << 16;
        uint32_t g = (col & 0x0000ff00);

        uint32_t newCol = a | r | g | b;

        *pDest = newCol;
        pDest++;
    }


    int ret = 0;
    if (sExt == ".png")
        ret = stbi_write_png(sName.c_str(), w, h, 4, pSwizzled,w * 4);
    else if (sExt == ".bmp")
        ret = stbi_write_bmp(sName.c_str(), w, h, 4, pSwizzled);
    else if (sExt == ".tga")
        ret = stbi_write_bmp(sName.c_str(), w, h, 4, pSwizzled);
    else if (sExt == ".jpg")
        ret = stbi_write_jpg(sName.c_str(), w, h, 4, pSwizzled, 80);
    else
    {
        ZERROR("ERROR: Unsupported image extension:", sName);
        delete[] pSwizzled;
        return false;
    }

    if (ret != 0)
    {
        ZERROR("ERROR: stb_image_write_xxx returned:", ret);
        delete[] pSwizzled;
        return false;
    }

    delete[] pSwizzled;
    return true;
}



bool ZBuffer::ReadEXIFFromFile(const std::string& sName, easyexif::EXIFInfo& info)
{
    std::filesystem::path filename(sName);
    if (!std::filesystem::exists(filename))
        return false;

    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        ZERROR("ERROR: Failed to open file:", sName);
        return false;
    }

    uint8_t marker[2];
    if (!file.read((char*)marker, 2))
    {
        ZERROR("ERROR: Failed to read initial 2 byte header from file:", sName);
        return false;
    }

    if (marker[0] != 0xff || marker[1] != 0xd8)
    {
        ZERROR("ERROR: Not a jpeg. Image header does not start with 0xffd8:", sName);
        return false;
    }  




    char segmentSize[2];
    uint16_t nSegBytes = 0;
    while (file.read((char*)marker, 2))
    {
        if (!file.read(segmentSize, 2))
        {
            ZERROR("ERROR: Failed to read segment size");
            return false;
        }

        nSegBytes = segmentSize[0] << 8 | segmentSize[1];
//        segmentSize = (segmentSize << 8) | (segmentSize >> 8);

        if (marker[0] == 0xff && marker[1] == 0xe1)
        {
            break;
        }

        file.seekg(nSegBytes - 2, std::ios::cur);
    }

//    ZOUT("Found exif segment at offset:", file.tellg());
    uint8_t* pBuf = new uint8_t[nSegBytes];

    file.read((char*)pBuf, nSegBytes);


    info.clear();
    int result = info.parseFromEXIFSegment(pBuf, nSegBytes);

    delete[] pBuf;
    if (result != 0)
    {
        ZDEBUG_OUT("No exif available. Code:", result);
        return false;
    }



    return true;
}







/*
#ifdef _WIN64
int GetEncoderClsid(const string& sFilename, CLSID* pClsid)
{
    string sExtension;
    size_t nLastDot = sFilename.rfind('.');
    if (nLastDot == std::string::npos)
    {
        cerr << "Couldn't extract extension from:" << sFilename << "\n";
        return -1;
    }

    sExtension = sFilename.substr(nLastDot + 1);
    SH::makelower(sExtension);

    wstring wsFormat;
    if (sExtension == "jpg" || sExtension == "jpeg")
        wsFormat = L"image/jpeg";
    else if (sExtension == "gif")
        wsFormat = L"image/gif";
    else if (sExtension == "bmp")
        wsFormat = L"image/bmp";
    else if (sExtension == "png")
        wsFormat = L"image/png";
    else if (sExtension == "tiff" || sExtension == "tif")
        wsFormat = L"image/tiff";
    else
    {
        ZDEBUG_OUT("Unsupported extension for filename:", sFilename);
        return -1;
    }

        




    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, wsFormat.c_str()) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

bool ZBuffer::SaveBuffer(const string& sFilename)
{
    BITMAPINFO bi;
    
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = (LONG)GetArea().Width();
    bi.bmiHeader.biHeight = (LONG)-GetArea().Height();
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    bi.bmiHeader.biXPelsPerMeter = 72;
    bi.bmiHeader.biYPelsPerMeter = 72;
    bi.bmiHeader.biClrUsed = bi.bmiHeader.biWidth * bi.bmiHeader.biHeight * 4;
    bi.bmiHeader.biClrImportant = 0;
//    bi.bmiColors = mpPixels;

    CLSID   encoderClsid;
       // Get the CLSID of the PNG encoder.
    GetEncoderClsid(sFilename, &encoderClsid);

    Bitmap bitmap(&bi, (void*) mpPixels);
    bitmap.Save(SH::string2wstring(sFilename).c_str(), (const CLSID*)&encoderClsid);

    return true;
}
#endif
*/

/*
#ifdef _WIN64
// TBD, do we want to keep loading from a resource?
bool ZBuffer::LoadBuffer(uint32_t nResourceID)
{
	Bitmap bitmap((HINSTANCE)::GetModuleHandle(NULL), MAKEINTRESOURCEW(nResourceID));

	Shutdown(); // Clear out any existing data

	if (!Init(bitmap.GetWidth(), bitmap.GetHeight()))
		return false;

	for (int64_t y = 0; y < (int64_t) bitmap.GetHeight(); y++)
	{
		for (int64_t x = 0; x < (int64_t) bitmap.GetWidth(); x++)
		{
			Color col = 0;
			bitmap.GetPixel((INT)x, (INT)y, &col);
			SetPixel(x, y, col.GetValue());
		}
	}

	return true;
}
#endif
*/

bool ZBuffer::Rotate(eOrientation rotation)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    uint32_t nPixels = (uint32_t) (mSurfaceArea.Width() * mSurfaceArea.Height());

    if (nPixels < 1)
        return false;

    if (rotation == kLeft || rotation == kLeftAndVFlip)
    {
        ZRect rOldArea(mSurfaceArea);
        uint32_t oldW = (uint32_t)rOldArea.Width();
        uint32_t oldH = (uint32_t)rOldArea.Height();
        uint32_t newW = oldH;
        uint32_t newH = oldW;
        ZRect rNewArea(0, 0, newW, newH);

        ZBuffer newBuf;
        newBuf.Init(newW, newH);

        for (uint32_t y = 0; y < oldH; y++)
        {
            for (uint32_t x = 0; x < oldW; x++)
            {
                newBuf.SetPixel(y, newH - x - 1, GetPixel(x, y));
            }
        }

        mSurfaceArea = rNewArea;
        memcpy(mpPixels, newBuf.GetPixels(), nPixels * sizeof(uint32_t));
    }
    if (rotation == kRight || rotation == kRightAndVFlip)
    {
        ZRect rOldArea(mSurfaceArea);
        uint32_t oldW = (uint32_t)rOldArea.Width();
        uint32_t oldH = (uint32_t)rOldArea.Height();
        uint32_t newW = oldH;
        uint32_t newH = oldW;
        ZRect rNewArea(0, 0, newW, newH);

        ZBuffer newBuf;
        newBuf.Init(newW, newH);

        for (uint32_t y = 0; y < oldH; y++)
        {
            for (uint32_t x = 0; x < oldW; x++)
            {
                newBuf.SetPixel(newW - y - 1, x, GetPixel(x, y));
            }
        }

        mSurfaceArea = rNewArea;
        memcpy(mpPixels, newBuf.GetPixels(), nPixels * sizeof(uint32_t));
    }
    if (rotation == k180)
    {
        ZBuffer newBuf;
        uint32_t newW = (uint32_t)mSurfaceArea.Width();
        uint32_t newH = (uint32_t)mSurfaceArea.Height();
        newBuf.Init(newW, newH);

        for (uint32_t y = 0; y < newH; y++)
        {
            for (uint32_t x = 0; x < newW; x++)
            {
                newBuf.SetPixel(newW - x - 1, newH - y - 1, GetPixel(x, y));
            }
        }

        memcpy(mpPixels, newBuf.GetPixels(), nPixels * sizeof(uint32_t));
    }
    if (rotation == kVFlip || rotation == kLeftAndVFlip || rotation == kRightAndVFlip)
    {
        ZBuffer newBuf;
        uint32_t newW = (uint32_t)mSurfaceArea.Width();
        uint32_t newH = (uint32_t)mSurfaceArea.Height();
        newBuf.Init(newW, newH);

        for (uint32_t y = 0; y < newH; y++)
        {
            for (uint32_t x = 0; x < newW; x++)
            {
                newBuf.SetPixel(x, newH - y - 1, GetPixel(x, y));
            }
        }

        memcpy(mpPixels, newBuf.GetPixels(), nPixels * sizeof(uint32_t));
    }
    if (rotation == kHFlip)
    {
        ZBuffer newBuf;
        uint32_t newW = (uint32_t) mSurfaceArea.Width();
        uint32_t newH = (uint32_t) mSurfaceArea.Height();
        newBuf.Init(newW, newH);

        for (uint32_t y = 0; y < newH; y++)
        {
            for (uint32_t x = 0; x < newW; x++)
            {
                newBuf.SetPixel(newW-x-1, y, GetPixel(x, y));
            }
        }

        memcpy(mpPixels, newBuf.GetPixels(), nPixels * sizeof(uint32_t));
    }

    return true;
}




bool ZBuffer::BltNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, eAlphaBlendType type)
{
    int64_t nSW = pSrc->GetArea().Width();
    int64_t nDW = mSurfaceArea.Width();

	int64_t nBltWidth = rDst.Width();
	int64_t nBltHeight = rDst.Height();

    if (pSrc->mbHasAlphaPixels)
    {
        uint32_t* pSrcBits = pSrc->GetPixels() + (rSrc.top * pSrc->GetArea().Width()) + rSrc.left;
        uint32_t* pDstBits = mpPixels + (rDst.top * mSurfaceArea.Width()) + rDst.left;
        
        for (int64_t y = 0; y < nBltHeight; y++)
        {
            for (int64_t x = 0; x < nBltWidth; x++)
            {
#ifdef _DEBUG
                //			CEASSERT_MESSAGE( pSrcBits < pSrc->GetPixels() + pSrc->GetArea().Width() * pSrc->GetArea().Height(), "Source buffer pointer out of range." );
                //			CEASSERT_MESSAGE( pDstBits < mpPixels + mSurfaceArea.Width() * mSurfaceArea.Height(), "Dest buffer pointer out of range." );
#endif

                uint8_t nAlpha = ARGB_A(*pSrcBits);
                if (nAlpha > 250)
                    *pDstBits = *pSrcBits;
                else if (nAlpha > 8)
                {
                    if (type == kAlphaDest)
                        *pDstBits = COL::AlphaBlend_Col2Alpha(*pSrcBits, *pDstBits, nAlpha);
                    else
                        *pDstBits = COL::AlphaBlend_Col1Alpha(*pSrcBits, *pDstBits, nAlpha);
                }

                pSrcBits++;
                pDstBits++;
            }

            pSrcBits += (pSrc->GetArea().Width() - nBltWidth);  // Next line in the source buffer
            pDstBits += (mSurfaceArea.Width() - nBltWidth);        // Next line in the destination buffer
        }
    }
    else
    {
        // no alpha pixels..... copy over rows of pixels directly
        for (int64_t y = 0; y < nBltHeight; y++)
        {
            uint32_t* pSrcBits = pSrc->mpPixels + ((y + rSrc.top) * nSW) + rSrc.left;
            uint32_t* pDstBits = mpPixels + ((y + rDst.top) * mSurfaceArea.Width()) + rDst.left;
            memcpy(pDstBits, pSrcBits, nBltWidth * 4);
        }
    }

    return true;
}


bool ZBuffer::BltAlphaNoClip(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, eAlphaBlendType type)
{
	if (nAlpha < 8)     // If less than a small threshhold, we won't see anything from the source buffer anyway
		return true;

//	if (nAlpha > 250)     // If close to 1, just do a plain blt
//		return BltNoClip(pSrc, rSrc, rDst);

	int64_t nBltWidth = rDst.Width();
	int64_t nBltHeight = rDst.Height();

	uint32_t* pSrcBits = pSrc->GetPixels() + (rSrc.top * pSrc->GetArea().Width()) + rSrc.left;
	uint32_t* pDstBits = mpPixels + (rDst.top * mSurfaceArea.Width()) + rDst.left;


	for (int64_t y = 0; y < nBltHeight; y++)
	{
		for (int64_t x = 0; x < nBltWidth; x++)
		{
			uint32_t nColSrc = *pSrcBits;
			if (ARGB_A(nColSrc) != 0)
			{
				if (type == kAlphaDest)
					*pDstBits = COL::AlphaBlend_Col2Alpha(*pSrcBits, *pDstBits, nAlpha);
				else
					*pDstBits = COL::AlphaBlend_Col1Alpha(*pSrcBits, *pDstBits, nAlpha);
			}

			pSrcBits++;
			pDstBits++;
		}

		pSrcBits += (pSrc->GetArea().Width() - nBltWidth);  // Next line in the source buffer
		pDstBits += (mSurfaceArea.Width() - nBltWidth);        // Next line in the destination buffer
	}


	return true;
}
/*inline
bool cCEBuffer::BltToNoClip(cCEBuffer* pDst, ZRect& rSrc, ZRect& rDst)
{
CEASSERT(!"No implemented");
return false;
}*/

bool ZBuffer::CopyPixels(ZBuffer* pSrc)
{
    if (pSrc->GetArea() != mSurfaceArea)
        return false;

    memcpy(mpPixels, pSrc->GetPixels(), mSurfaceArea.Width() * mSurfaceArea.Height() * 4);
    return true;
}

bool ZBuffer::CopyPixels(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip)
{
    if (pSrc->GetArea() == mSurfaceArea && rSrc == mSurfaceArea && rDst == mSurfaceArea)
    {
        memcpy(mpPixels, pSrc->GetPixels(), mSurfaceArea.Width() * mSurfaceArea.Height() * 4);
        return true;
    }

	ZRect rSrcClipped(rSrc);
	ZRect rDstClipped(rDst);

	ZRect rClip;
	if (pClip)
	{
		rClip.SetRect(*pClip);
		rClip.IntersectRect(&mSurfaceArea);
	}
	else
		rClip.SetRect(mSurfaceArea);

	if (Clip(rClip, rSrcClipped, rDstClipped))
	{
		int64_t nBltWidth = rDst.Width();
		int64_t nBltHeight = rDst.Height();

		uint32_t* pSrcBits = pSrc->GetPixels() + (rSrc.top * pSrc->GetArea().Width()) + rSrc.left;
		uint32_t* pDstBits = mpPixels + (rDst.top * mSurfaceArea.Width()) + rDst.left;

		for (int64_t y = 0; y < nBltHeight; y++)
		{
			memcpy(pDstBits, pSrcBits, nBltWidth * sizeof(uint32_t));
/*			for (int64_t x = 0; x < nBltWidth; x++)
			{
				*pDstBits = *pSrcBits;

				pSrcBits++;
				pDstBits++;
			}*/

			pSrcBits += (pSrc->GetArea().Width());  // Next line in the source buffer
			pDstBits += (mSurfaceArea.Width());        // Next line in the destination buffer
		}

		return true;
	}
	return false;
}


bool ZBuffer::Blt(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, ZRect* pClip, eAlphaBlendType type)
{
	ZRect rSrcClipped(rSrc);
	ZRect rDstClipped(rDst);

	ZRect rClip;
	if (pClip)
	{
		rClip.SetRect(*pClip);
		rClip.IntersectRect(&mSurfaceArea);
	}
	else
		rClip.SetRect(mSurfaceArea);

    if (Clip(pSrc->GetArea(), rClip, rSrcClipped, rDstClipped))
        return BltNoClip(pSrc, rSrcClipped, rDstClipped, type);

	return false;
}

bool ZBuffer::BltAlpha(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, uint32_t nAlpha, ZRect* pClip, eAlphaBlendType type)
{
	ZRect rSrcClipped(rSrc);
	ZRect rDstClipped(rDst);

	ZRect rClip;
	if (pClip)
	{
		rClip.SetRect(*pClip);
		rClip.IntersectRect(&mSurfaceArea);
	}
	else
		rClip.SetRect(mSurfaceArea);

	if (Clip(rClip, rSrcClipped, rDstClipped))
		return BltAlphaNoClip(pSrc, rSrcClipped, rDstClipped, nAlpha, type);
	return false;
}

bool ZBuffer::Fill(uint32_t nCol, ZRect* pRect)
{
    ZRect rDst;
    if (pRect)
    {
        rDst.SetRect(*pRect);
        if (!Clip(rDst))
            return false;
    }
    else
    {
        rDst.SetRect(mSurfaceArea);
/*        size_t numPixels = mSurfaceArea.Area();
        __stosd((DWORD*)mpPixels, nCol, numPixels);
        return true;*/
    }

	int64_t nDstStride = mSurfaceArea.Width();

	int64_t nFillWidth = rDst.Width();
	int64_t nFillHeight = rDst.Height();


	// Fully opaque
	for (int64_t y = 0; y < nFillHeight; y++)
	{
		uint32_t* pDstBits = (uint32_t*) (mpPixels + ((y + rDst.top) * nDstStride) + rDst.left);
/*		for (int64_t x = 0; x < nFillWidth; x++)
			*pDstBits++ = nCol;*/
        __stosd((DWORD*)pDstBits, nCol, nFillWidth);
	}

    mbHasAlphaPixels = ARGB_A(nCol) < 255;

	return true;
}

bool ZBuffer::FillAlpha(uint32_t nCol, ZRect* pRect)
{
    ZRect rDst;
    if (pRect)
    {
        rDst.SetRect(*pRect);
        if (!Clip(rDst))
            return false;
    }
    else
        rDst.SetRect(mSurfaceArea);


	int64_t nDstStride = mSurfaceArea.Width();

	int64_t nFillWidth = rDst.Width();
	int64_t nFillHeight = rDst.Height();


	int64_t nAlpha = ARGB_A(nCol);
	if (nAlpha > 250)
	{
		// Fully opaque
		for (int64_t y = 0; y < nFillHeight; y++)
		{
			uint32_t* pDstBits = (uint32_t*) (mpPixels + ((y + rDst.top) * nDstStride) + rDst.left);
//			for (int64_t x = 0; x < nFillWidth; x++)
//				*pDstBits++ = nCol;
            __stosd((DWORD*)pDstBits, nCol, nFillWidth);

		}
	}
	else if (nAlpha > 8)
	{
		for (int64_t y = 0; y < nFillHeight; y++)
		{
			uint32_t* pDstBits = (uint32_t*) (mpPixels + ((y + rDst.top) * nDstStride) + rDst.left);
			for (int64_t x = 0; x < nFillWidth; x++)
			{
				*pDstBits++ = COL::AlphaBlend_Col2Alpha(nCol, *pDstBits, ARGB_A(nCol));
			}
		}
	}

	return true;
}

bool ZBuffer::FillGradient(uint32_t nCol[4], ZRect* pRect)
{
    ZRect rDst;
    if (pRect)
    {
        rDst.SetRect(*pRect);
        if (!Clip(rDst))
            return false;
    }
    else
        rDst.SetRect(mSurfaceArea);

    tColorVertexArray array;
    gRasterizer.RectToVerts(rDst, array);

    array[0].mColor = nCol[0];
    array[1].mColor = nCol[1];
    array[2].mColor = nCol[2];
    array[3].mColor = nCol[3];

    return gRasterizer.Rasterize(this, array);
}


bool ZBuffer::Colorize(uint32_t nH, uint32_t nS, ZRect* pRect)
{
    ZRect rDst;
    if (pRect)
    {
        rDst.SetRect(*pRect);
        if (!Clip(rDst))
            return false;
    }
    else
        rDst.SetRect(mSurfaceArea);

    int64_t nDstStride = mSurfaceArea.Width();

    int64_t nFillWidth = rDst.Width();
    int64_t nFillHeight = rDst.Height();


    for (int64_t y = 0; y < nFillHeight; y++)
    {
        uint32_t* pDstBits = (uint32_t*)(mpPixels + ((y + rDst.top) * nDstStride) + rDst.left);
        for (int64_t x = 0; x < nFillWidth; x++)
        {
            uint32_t nCol = *pDstBits;
            uint32_t nHSV = COL::ARGB_To_AHSV(nCol);
            uint8_t  a = AHSV_A(nHSV);
            uint32_t h = AHSV_H(nHSV);
            uint32_t s = AHSV_S(nHSV);
            uint32_t v = AHSV_V(nHSV);
            nCol = COL::AHSV_To_ARGB(a, nH, nS, v);

           *pDstBits++ = nCol;
        }
    }

    return true;

}


void  ZBuffer::DrawRectAlpha(uint32_t nCol, ZRect rRect)
{
    // Bottom and right are inclusive, so -1
    rRect.right--;
    rRect.bottom--;

    ZRect rClipped(rRect.left, rRect.top, rRect.right, rRect.bottom);  
    if (Clip(rClipped))
    {
        int64_t nDstStride = mSurfaceArea.Width();
        int64_t nFillWidth = rClipped.Width();
        int64_t nFillHeight = rClipped.Height();
        int64_t nAlpha = ARGB_A(nCol);

        uint32_t* pDstBits;

        // draw top 
        if (rRect.top >= mSurfaceArea.top)   // if unclipped top is visible
        {
            pDstBits = (uint32_t*)(mpPixels + (rClipped.top * nDstStride) + rClipped.left);
            for (int64_t x = 0; x < nFillWidth; x++)
            {
                *pDstBits++ = COL::AlphaBlend_Col2Alpha(nCol, *pDstBits, ARGB_A(nCol));
            }
        }

        // draw left
        if (rRect.left >= mSurfaceArea.left) // if unclipped left is visible
        {
            pDstBits = (uint32_t*)(mpPixels + (rClipped.top * nDstStride) + rClipped.left);
            for (int64_t y = 0; y < nFillHeight; y++)
            {
                *pDstBits = COL::AlphaBlend_Col2Alpha(nCol, *pDstBits, ARGB_A(nCol));
                pDstBits += nDstStride;
            }
        }

        // draw right
        if (rRect.right < mSurfaceArea.right)   // if unclipped right is visible
        {
            pDstBits = (uint32_t*)(mpPixels + (rClipped.top * nDstStride) + rClipped.right);
            for (int64_t y = 0; y < nFillHeight; y++)
            {
                *pDstBits = COL::AlphaBlend_Col2Alpha(nCol, *pDstBits, ARGB_A(nCol));
                pDstBits += nDstStride;
            }
        }

        // draw bottom
        if (rRect.bottom < mSurfaceArea.bottom)     // if unclipped bottom is visible
        {
            pDstBits = (uint32_t*)(mpPixels + (rClipped.bottom * nDstStride) + rClipped.left);
            for (int64_t x = 0; x < nFillWidth; x++)
            {
                *pDstBits++ = COL::AlphaBlend_Col2Alpha(nCol, *pDstBits, ARGB_A(nCol));
            }
        }
    }
}

void ZBuffer::DrawCircle(ZPoint center, int64_t radius, uint32_t col)
{
    int64_t startScanline = center.y - radius;
    limit<int64_t>(startScanline, 0, mSurfaceArea.bottom);

    int64_t endScanline = center.y + radius;
    limit<int64_t>(endScanline, 0, mSurfaceArea.bottom);

    int64_t nDstStride = mSurfaceArea.Width();

    for (int64_t y = startScanline; y < endScanline; y++)
    {
        int64_t dy = abs(y - center.y);
        int64_t dx = (int64_t)sqrt(radius * radius - (dy * dy));
        int64_t dx_start = center.x - dx;
        int64_t dx_end = center.x + dx;
        limit<int64_t>(dx_start, 0, mSurfaceArea.right);
        limit<int64_t>(dx_end, 0, mSurfaceArea.right);


        uint32_t* pDstBits = (uint32_t*)(mpPixels + (y * nDstStride) + dx_start);

        double fA = (double)(col >> 24);
        double fR = (double)((col & 0x00ff0000) >> 16);
        double fG = (double)((col & 0x0000ff00) >> 8);
        double fB = (double)((col & 0x000000ff));

        FillInSpan(pDstBits, dx_end - dx_start, fR, fG, fB, fA);
    }
}

void ZBuffer::DrawSphere(ZPoint center, int64_t radius, const Z3D::Vec3f& lightPos, const Z3D::Vec3f& viewPos, const Z3D::Vec3f& ambient, const Z3D::Vec3f& diffuse, const Z3D::Vec3f& specular, float shininess)
{
    int64_t startScanline = center.y - radius;

    int64_t endScanline = center.y + radius;

    int64_t nDstStride = mSurfaceArea.Width();


//    startScanline = center.y - 1;
//    endScanline = center.y;

    limit<int64_t>(endScanline, 0, mSurfaceArea.bottom);
    limit<int64_t>(startScanline, 0, mSurfaceArea.bottom);


    for (int64_t y = startScanline; y < endScanline; y++)
    {
        int64_t dy = abs(y - center.y);
        int64_t dx = (int64_t)sqrt(radius * radius - (dy * dy));
        int64_t x = center.x - dx;
        int64_t x_end = center.x + dx;
        limit<int64_t>(x, 0, mSurfaceArea.right);
        limit<int64_t>(x_end, 0, mSurfaceArea.right);


        uint32_t* pDest = (uint32_t*)(mpPixels + (y * nDstStride) + x);
//        FillInSpan(pDstBits, dx_end - dx_start, fR, fG, fB, fA);
        int64_t nNumPixels = x_end - x;
        while (nNumPixels > 0)
        {
            uint8_t nCurA;
            uint8_t nCurR;
            uint8_t nCurG;
            uint8_t nCurB;

            uint32_t col = Z3D::calculateLighting((float)x, (float)y, (float)center.x, (float)center.y, (float)radius, lightPos, viewPos, ambient, diffuse, specular, shininess);

            float fA = (float)(col >> 24);
            float fR = (float)((col & 0x00ff0000) >> 16);
            float fG = (float)((col & 0x0000ff00) >> 8);
            float fB = (float)((col & 0x000000ff));


            TO_ARGB(*pDest, nCurA, nCurR, nCurG, nCurB);


            uint8_t nDestR = static_cast<uint8_t>((uint32_t)((fR * fA + (nCurR * (255.0f - fA)))) >> 8);
            uint8_t nDestG = static_cast<uint8_t>((uint32_t)((fG * fA + (nCurG * (255.0f - fA)))) >> 8);
            uint8_t nDestB = static_cast<uint8_t>((uint32_t)((fB * fA + (nCurB * (255.0f - fA)))) >> 8);

            *pDest = ARGB(255, nDestR, nDestG, nDestB);

            pDest++;
            x++;
            nNumPixels--;
        }

    }
}





bool ZBuffer::Clip(ZRect& dstRect)
{
    int64_t nOverhangDistance;

    if ((nOverhangDistance = dstRect.right - mSurfaceArea.right) > 0)
    {
        dstRect.right -= nOverhangDistance;
        if (dstRect.right <= dstRect.left)
            return false; //The dest rect is entirely outside of clipping area.
    }
    if ((nOverhangDistance = mSurfaceArea.left - dstRect.left) > 0)
    {
        dstRect.left += nOverhangDistance;
        if (dstRect.left >= dstRect.right)
            return false; //The dest rect is entirely outside of clipping area.
    }
    if ((nOverhangDistance = dstRect.bottom - mSurfaceArea.bottom) > 0)
    {
        dstRect.bottom -= nOverhangDistance;
        if (dstRect.bottom <= dstRect.top)
            return false; //The dest rect is entirely outside of clipping area.
    }
    if ((nOverhangDistance = mSurfaceArea.top - dstRect.top) > 0)
    {
        dstRect.top += nOverhangDistance;
        if (dstRect.top >= dstRect.bottom)
            return false; //The dest rect is entirely outside of clipping area.
    }

    return true;
}


bool ZBuffer::Clip(const ZRect& fullDstRect, ZRect& srcRect, ZRect& dstRect)
{
	int64_t   nOverhangDistance;  
	ZRect	rBufferArea(fullDstRect);

	//Clip Dst
	if((nOverhangDistance = dstRect.right - rBufferArea.right) > 0)
	{
		dstRect.right -= nOverhangDistance;
		srcRect.right  -= nOverhangDistance;
		if(dstRect.right <= dstRect.left)
			return false; //The dest rect is entirely outside of clipping area.
	}
	if((nOverhangDistance = rBufferArea.left - dstRect.left) > 0)
	{
		dstRect.left += nOverhangDistance;
		srcRect.left  += nOverhangDistance;
		if(dstRect.left >= dstRect.right)
			return false; //The dest rect is entirely outside of clipping area.
	}
	if((nOverhangDistance = dstRect.bottom - rBufferArea.bottom) > 0)
	{
		dstRect.bottom -= nOverhangDistance;
		srcRect.bottom  -= nOverhangDistance;
		if(dstRect.bottom <= dstRect.top)
			return false; //The dest rect is entirely outside of clipping area.
	}
	if((nOverhangDistance = rBufferArea.top - dstRect.top) > 0)
	{
		dstRect.top += nOverhangDistance;
		srcRect.top  += nOverhangDistance;
		if(dstRect.top >= dstRect.bottom)
			return false; //The dest rect is entirely outside of clipping area.
	}

	return true;
}


bool ZBuffer::Clip(const ZRect& rSrcSurface, const ZRect& rDstSurface, ZRect& rSrc, ZRect& rDst)
{
    int64_t   nOverhangDistance;

    // rDst against rDstSurface 
    // RIGHT 
    if ((nOverhangDistance = rDst.right - rDstSurface.right) > 0)
    {
        rDst.right -= nOverhangDistance;
        rSrc.right -= nOverhangDistance;
    }

    // LEFT  
    if ((nOverhangDistance = rDstSurface.left - rDst.left) > 0)
    {
        rDst.left += nOverhangDistance;
        rSrc.left += nOverhangDistance;
    }

    // BOTTOM  
    if ((nOverhangDistance = rDst.bottom - rDstSurface.bottom) > 0)
    {
        rDst.bottom -= nOverhangDistance;
        rSrc.bottom -= nOverhangDistance;
    }

    // TOP  
    if ((nOverhangDistance = rDstSurface.top - rDst.top) > 0)
    {
        rDst.top += nOverhangDistance;
        rSrc.top += nOverhangDistance;
    }

    // rSrc against rSrcSurface
    if ((nOverhangDistance = rSrc.right - rSrcSurface.right) > 0)
    {
        rDst.right -= nOverhangDistance;
        rSrc.right -= nOverhangDistance;
    }

    // LEFT  
    if ((nOverhangDistance = rSrcSurface.left - rSrc.left) > 0)
    {
        rDst.left += nOverhangDistance;
        rSrc.left += nOverhangDistance;
    }

    // BOTTOM  
    if ((nOverhangDistance = rSrc.bottom - rSrcSurface.bottom) > 0)
    {
        rDst.bottom -= nOverhangDistance;
        rSrc.bottom -= nOverhangDistance;
    }

    // TOP  
    if ((nOverhangDistance = rSrcSurface.top - rSrc.top) > 0)
    {
        rDst.top += nOverhangDistance;
        rSrc.top += nOverhangDistance;
    }

    return rSrc.Width() > 0 && rSrc.Height() > 0 && rDst.Width() > 0 && rDst.Height() > 0;
}

inline
bool ZBuffer::Clip(const ZBuffer* pSrc, const ZBuffer* pDst, ZRect& rSrc, ZRect& rDst)
{
    return Clip(pSrc->mSurfaceArea, pDst->mSurfaceArea, rSrc, rDst);
}


inline
uint32_t ZBuffer::GetPixel(int64_t x, int64_t y)
{
	return *(mpPixels + y * mSurfaceArea.right + x);
}

inline
void ZBuffer::SetPixel(int64_t x, int64_t y, uint32_t nCol)
{
	*(mpPixels + y * mSurfaceArea.right + x) = nCol;
}

bool ZBuffer::BltTiled(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, int64_t nStartX, int64_t nStartY, ZRect* pClip, eAlphaBlendType type)
{
	int64_t nSrcWidth = rSrc.Width();
	int64_t nSrcHeight = rSrc.Height();

	//CEASSERT(nSrcHeight > 0 && nSrcWidth > 0);
	if (nSrcHeight <= 0 || nSrcWidth <= 0)
		return false;

	ZRect rClip(rDst);
	if (pClip)
	{
		if (!rClip.IntersectRect(pClip))
		{
			return false;
		}
	}

	for (int64_t y = rDst.top-nStartY; y < rDst.bottom; y+= nSrcHeight)
	{
		for (int64_t x = rDst.left-nStartX; x < rDst.right; x+= nSrcWidth)
		{
			ZRect rCurrentDst(x, y, x + nSrcWidth, y + nSrcHeight);
			Blt(pSrc, rSrc, rCurrentDst, &rClip, type);
		}
	}

	return true;
}


// Source Bitmap is split into 9 regions
//
//             w
// /-----------^------------\
//         |       |        |             
//    TL   |   T   |   TR   |              
//         |       |        |              
//  -------+---t---+--------|             
//         |       |        |              
//     L   l   M   r   R    >h             
//         |       |        |              
//  -------+---b---+--------|             
//         |       |        |              
//    BL   |   B   |   BR   |             
//         |       |        /              
//                                        
//  The variables l,t,r,b are passed in the rEdgeRect
//
//  This function Blts the TL, TR, BL, BR corners into the destination corners
//  The edges are tiled into the destination edges
//  The Middle section is tile blt if so desired
//
//
//
//    Destination regions are as follows
//
//            dw
// /-----------^------------\
//         |       |        |              
//    dTL  |   dT  |  dTR   |               
//         |       |        |               
//  -------+--dt---+--------|              
//         |       |        |               
//    dL  dl  dM  dr  dR    >dh
//         |       |        |               
//  -------+--db---+--------|              
//         |       |        |               
//   dBL   |  dB   |  dBR   |              
//         |       |        |               
//                                        
bool ZBuffer::BltEdge(ZBuffer* pSrc, ZRect& rEdgeRect, ZRect& rDst, uint32_t flags, ZRect* pClip, eAlphaBlendType type)
{
	ZASSERT(pSrc);

	ZRect rClip(rDst);
	if (pClip)
	{
		if (!rClip.IntersectRect(pClip))
			return false;
	}

	// Set up the Source Rectangles
	int64_t w = pSrc->GetArea().Width();
	int64_t h = pSrc->GetArea().Height();

	int64_t l = rEdgeRect.left;
	int64_t t = rEdgeRect.top;
	int64_t r = rEdgeRect.right;
	int64_t b = rEdgeRect.bottom;

	// Adjustments for destinations whos width or height are smaller than the source rect
	if (rDst.Height() < t+b)
	{
		t = rDst.Height()/2;
		b = h - t;
	}

	if (rDst.Width() < l+r)
	{
		l = rDst.Width()/2;
		r = w - l;
	}

	ZRect rTL(0, 0, l, t);
	ZRect rTR(r, 0, w, t);
	ZRect rBL(0, b, l, h);
	ZRect rBR(r, b, w, h);

	ZRect rT(l, 0, r, t);
	ZRect rL(0, t, l, b);
	ZRect rR(r, t, w, b);
	ZRect rB(l, b, r, h);

	ZRect rM(l,t,r,b);


	// Set up the Destination Rectangles
	int64_t dl = rDst.left + l;
	int64_t dt = rDst.top + t;
	int64_t dr = rDst.right - (w-r);
	int64_t db = rDst.bottom - (h-b);

	ZRect rdTL(rDst.left, rDst.top, dl, dt);
	ZRect rdTR(dr, rDst.top, rDst.right, dt);
	ZRect rdBL(rDst.left, db, dl, rDst.bottom);
	ZRect rdBR(dr,db, rDst.right, rDst.bottom);
     
	ZRect rdT(dl, rDst.top, dr, dt);
	ZRect rdL(rDst.left, dt, dl, db);
	ZRect rdR(dr, dt, rDst.right, db);
	ZRect rdB(dl, db, dr, rDst.bottom);

	ZRect rdM(dl, dt, dr, db);

	bool bDrawTop		= rdTL.Height()>0;
	bool bDrawLeft		= rdTL.Width()>0;
	bool bDrawBottom	= rdBR.Height()>0;
	bool bDrawRight		= rdBR.Width()>0;

	if (bDrawTop && bDrawLeft)
		Blt(pSrc, rTL, rdTL, &rClip, type);

	if (bDrawTop && bDrawRight)
		Blt(pSrc, rTR, rdTR, &rClip, type);

	if (bDrawBottom && bDrawLeft)
		Blt(pSrc, rBL, rdBL, &rClip, type);

    if (bDrawBottom && bDrawRight)
        Blt(pSrc, rBR, rdBR, &rClip, type);


    if ((flags & kEdgeBltSides_Stretch) != 0)
    {
        if (bDrawTop)
            gRasterizer.RasterizeWithAlphaSimple(this, pSrc, rdT, rT, &rClip);

        if (bDrawLeft)
            gRasterizer.RasterizeWithAlphaSimple(this, pSrc, rdL, rL, &rClip);

        if (bDrawRight)
            gRasterizer.RasterizeWithAlphaSimple(this, pSrc, rdR, rR, &rClip);

        if (bDrawBottom)
            gRasterizer.RasterizeWithAlphaSimple(this, pSrc, rdB, rB, &rClip);
    }
    else
    {
        if (bDrawTop)
            BltTiled(pSrc, rT, rdT, 0, 0, &rClip, type);

        if (bDrawLeft)
            BltTiled(pSrc, rL, rdL, 0, 0, &rClip, type);

        if (bDrawRight)
            BltTiled(pSrc, rR, rdR, 0, 0, &rClip, type);

        if (bDrawBottom)
            BltTiled(pSrc, rB, rdB, 0, 0, &rClip, type);
    }

    if (rdM.Width() > 0 && rdM.Height() > 0)
    {
        if ((flags&kEdgeBltMiddle_Tile) != 0)
        {
            BltTiled(pSrc, rM, rdM, 0, 0, &rClip, type);
        }
        else if ((flags&kEdgeBltMiddle_Stretch) != 0)
        {
            gRasterizer.RasterizeSimple(this, pSrc, rdM, rEdgeRect, &rClip);
        }
    }

	return true;
}

inline
bool ZBuffer::FloatScanLineIntersection(double fScanLine, const ZColorVertex& v1, const ZColorVertex& v2, double& fIntersection, double& fR, double& fG, double& fB, double& fA)
{
	if (v1.y == v2.y || v1.x == v2.x)	
	{
		fIntersection = v1.x;
		fA = (double) (v1.mColor >> 24);
		fR = (double) ((v1.mColor & 0x00ff0000) >> 16);
		fG = (double) ((v1.mColor & 0x0000ff00) >> 8);
		fB = (double) ((v1.mColor & 0x000000ff));
		return false;			
	}

	// Calculate line intersection
	double fM = (double)((v1.y - v2.y)/(v1.x - v2.x)); // slope
	fIntersection = (v2.x + (fScanLine-v2.y)/fM);

	double fT = (fScanLine - v1.y) / (v2.y - v1.y);  // how far along line (0.0 - 1.0)

	// Calculate color at intersection
	int64_t nA1 = (v1.mColor) >> 24;
	int64_t nA2 = (v2.mColor) >> 24;

	int64_t nR1 = (v1.mColor & 0x00ff0000) >> 16;
	int64_t nR2 = (v2.mColor & 0x00ff0000) >> 16;

	int64_t nG1 = (v1.mColor & 0x0000ff00) >> 8;
	int64_t nG2 = (v2.mColor & 0x0000ff00) >> 8;

	int64_t nB1 = (v1.mColor & 0x000000ff);
	int64_t nB2 = (v2.mColor & 0x000000ff);


	fA = (double) (nA1 + (nA2 - nA1) * fT);
	fR = (double) (nR1 + (nR2 - nR1) * fT);
	fG = (double) (nG1 + (nG2 - nG1) * fT);
	fB = (double) (nB1 + (nB2 - nB1) * fT);

	return true;
}

inline 
void ZBuffer::FillInSpan(uint32_t* pDest, int64_t nNumPixels, double fR, double fG, double fB, double fA)
{
	while (nNumPixels > 0)
	{
		uint8_t nCurA;
		uint8_t nCurR;
		uint8_t nCurG;
		uint8_t nCurB;
        TO_ARGB(*pDest, nCurA, nCurR, nCurG, nCurB);


		uint8_t nDestR = static_cast<uint8_t>((uint32_t) ((fR * fA + (nCurR * (255.0f-fA)))) >> 8);
		uint8_t nDestG = static_cast<uint8_t>((uint32_t) ((fG * fA + (nCurG * (255.0f-fA)))) >> 8);
		uint8_t nDestB = static_cast<uint8_t>((uint32_t) ((fB * fA + (nCurB * (255.0f-fA)))) >> 8);

		*pDest = ARGB(255, nDestR, nDestG, nDestB);

		pDest++;
		nNumPixels--;
	} 
}

void ZBuffer::DrawAlphaLine(const ZColorVertex& v1, const ZColorVertex& v2, double thickness, ZRect* pClip)
{
	ZRect rDest;

	if (pClip)
	{
		rDest = *pClip;
		rDest.IntersectRect(&mSurfaceArea);
	}
	else
		rDest = mSurfaceArea;

	ZRect rLineRect;

	rLineRect.top = (int64_t) min(v1.y, v2.y);
	rLineRect.bottom = (int64_t) max(v1.y, v2.y);
	rLineRect.left = (int64_t) min(v1.x - thickness /2.0f, v2.x - thickness /2.0f);
	rLineRect.right = (int64_t) max(v1.x + thickness /2.0f, v2.x + thickness /2.0f);

	rLineRect.IntersectRect(/*&rLineRect, */&rDest);

	uint32_t* pSurface = GetPixels();
	int64_t nStride = mSurfaceArea.right;

	double fScanLine = (double) rLineRect.top;
	double fIntersection;
	double fR;
	double fG;
	double fB;
	double fA;

	double fPrevScanLineIntersection;
	if (!FloatScanLineIntersection(fScanLine, v1, v2, fPrevScanLineIntersection, fR, fG, fB, fA) && v1.y == v2.y)
	{
		// Horizontal line is a special case:
		uint32_t* pDest = pSurface + rLineRect.top * nStride + rLineRect.left;
		FillInSpan(pDest, rLineRect.Width(), fR, fG, fB, fA);
	}

    for (int64_t nScanLine = rLineRect.top; nScanLine < rLineRect.bottom; nScanLine++)
	{
		FloatScanLineIntersection(fScanLine, v1, v2, fIntersection, fR, fG, fB, fA);

		if (fA > 10.0f)		// Anything less is not really visible
		{
			int64_t nStartPixel = (int64_t) min(fIntersection - thickness /2.0f, fPrevScanLineIntersection - thickness /2.0f);
			nStartPixel = max(nStartPixel, rLineRect.left);		// clip left

			int64_t nEndPixel = (int64_t) max(fIntersection + thickness /2.0f, fPrevScanLineIntersection + thickness /2.0f);
			nEndPixel = min(nEndPixel, rLineRect.right);

			uint32_t* pDest = pSurface + nScanLine * nStride + nStartPixel;

			FillInSpan(pDest, nEndPixel - nStartPixel, fR, fG, fB, fA);
		}

		fPrevScanLineIntersection = fIntersection;
		fScanLine += 1.0f;
	}
}


inline void TransformPoint(ZFPoint& pt, const ZFPoint& ptOrigin, double angle, double fScale)
{
	const double x(pt.x - ptOrigin.x);
	const double y(pt.y - ptOrigin.y);
	const double cosAngle((double)::cos(angle));
	const double sinAngle((double)::sin(angle));

	pt.x = ptOrigin.x + fScale*(x*cosAngle - y*sinAngle);
	pt.y = ptOrigin.y + fScale*(x*sinAngle + y*cosAngle);
}

bool ZBuffer::BltRotated(ZBuffer* pSrc, ZRect& rSrc, ZRect& rDst, double fAngle, double fScale, ZRect* pClip)
{
/*	ZUVVertex vert;
	vert.mfX = rSrc.left;
	vert.mfY = rSrc.

	tVertexArray vertArray;
	vertArray.push_back(vert);

	vert.mfX = rSrc.right;
}

/*bool cCEBuffer::BltRotated(cCEBuffer* pSrc, ZRect& rSrc, ZRect& rDst, float fAngle, float fScale, ZRect* pClip)
{*/
	int64_t               i, iEnd;
	ZRect				rDstActual;
	int64_t            numRows; 
	const             uint32_t kStackRowCount = 2048;
	int64_t            spanMinStack[kStackRowCount];  //With this, we eliminate the overhead of malloc for most cases.
	int64_t            spanMaxStack[kStackRowCount];  //This speeds things up and relaxes life on the memory manager.
	int64_t*           spanMinHeap = 0;
	int64_t*           spanMaxHeap = 0;
	int64_t*           spanMin;
	int64_t*           spanMax;
	ZFPoint center((rSrc.left+rSrc.right)/2.0F, //Center of source rect
		(rSrc.top+rSrc.bottom)/2.0F);
	ZFPoint destCenter(rDst.Width()/2.0F,          //Center of dest rect, but in units relative to the left 
		rDst.Height()/2.0F);        //  of the rect, and not the left of the destination buffer.
	ZFPoint origin(0, 0);                              //
	ZFPoint       dTex(1, 0);                                //
	ZFPoint       p[4];

	ZASSERT(pSrc);

	rDstActual = rDst;
	rDstActual.IntersectRect(&mSurfaceArea);
	if(pClip)
		rDstActual.IntersectRect(pClip);

	numRows = rDstActual.Height();
	if(numRows > kStackRowCount)
	{
		spanMin = spanMinHeap = new int64_t[numRows]; //We try to avoid doing this whenever possible.
		spanMax = spanMaxHeap = new int64_t[numRows];
	}
	else
	{
		spanMin = spanMinStack;
		spanMax = spanMaxStack;
	}

	// Init the spans to values that are at the extremes.
	for(i=0; i<numRows; i++)
	{   
		spanMin[i] = 0;
		spanMax[i] = rDstActual.Width();
	}

	// For each segment on the rotated rectangle, rasterize the line setting min and max if necessary
	p[0].x = (double)rSrc.left;    //Store the four corners of rectSource as four points.
	p[0].y = (double)rSrc.top;

	p[1].x = (double)rSrc.left;
	p[1].y = (double)rSrc.bottom;

	p[2].x = (double)rSrc.right;
	p[2].y = (double)rSrc.bottom;

	p[3].x = (double)rSrc.right;
	p[3].y = (double)rSrc.top;

	TransformPoint(p[0], center, fAngle, fScale);   //Rotate these points around the center of rectSource.
	TransformPoint(p[1], center, fAngle, fScale);
	TransformPoint(p[2], center, fAngle, fScale);
	TransformPoint(p[3], center, fAngle, fScale);

	p[0].x += destCenter.x - center.x;             //Now translate these points to be relative to destRect, with destRect.left
	p[0].y += destCenter.y - center.y;             //being 0. As if destRect had it's own coordinate system.
	p[0].x += rDst.left - rDstActual.left; //Then translate these points to be relative to destRectActual
	p[0].y += rDst.top  - rDstActual.top;  //  (clipped destination rectangle) instead of just destRect.
	//Now it's as if destRectActual had it's own coordinate system.
	p[1].x += destCenter.x - center.x;
	p[1].y += destCenter.y - center.y;
	p[1].x += rDst.left - rDstActual.left;
	p[1].y += rDst.top  - rDstActual.top;

	p[2].x += destCenter.x - center.x;
	p[2].y += destCenter.y - center.y;
	p[2].x += rDst.left - rDstActual.left;
	p[2].y += rDst.top  - rDstActual.top;

	p[3].x += destCenter.x - center.x;
	p[3].y += destCenter.y - center.y;
	p[3].x += rDst.left - rDstActual.left;
	p[3].y += rDst.top  - rDstActual.top;

	//For each destination scan line from top to bottom, test for 
	//  intersection against the four segments (edges). When we get done with this
	//  loop, we should have the destination left and right edges for each horizontal
	//  scan line in the destination. After that, we simply write in these spans with
	//  values taken from the source buffer. 
	//
	for(i=0, iEnd=rDstActual.Height(); i<iEnd; i++)
	{
		double m;      //slope of the current line.
		int64_t   xInt;   //x-Intercept of the current line with the beginning of the scan line.

		// segment 0-1 intersection
		if ((p[0].y <= i && p[1].y > i) || //If this scan line vertically is in the same range as the 0-1 line.
			(p[1].y <= i && p[0].y > i))
		{
			if (p[0].x == p[1].x)           //If the 0-1 line is vertical, the possible xIntercept is at x.
				xInt = (int) p[0].x;
			else{
				m = (double)((p[0].y - p[1].y)/(p[0].x - p[1].x));       //Calculate the intersection between the current
				xInt = (int64_t)(p[1].x + (double(i)-p[1].y)/m + 1.0F); //line and the current scan-line.
			} //The p0-p1 line crosses the current 'i' horizontal at x = xInt.
			if (xInt < spanMin[i]) //If this xIntercept is lower than the previous lowest xIntercept. 
				spanMin[i] = xInt;  //Since this is the first run through this, the test should always 
			if (xInt > spanMax[i]) //test as true.
				spanMax[i] = xInt;
		}

		// segment 1-2 intersection
		if ((p[1].y <= i && p[2].y > i) ||
			(p[2].y <= i && p[1].y > i))
		{
			if (p[1].x == p[2].x)
				xInt = (int) p[1].x;
			else{
				m = (double)((p[1].y - p[2].y)/(p[1].x - p[2].x));
				xInt = (int64_t)(p[2].x + (double(i)-p[2].y)/m + 1.0F);
			}
			if (xInt < spanMin[i])
				spanMin[i] = xInt;
			if (xInt > spanMax[i])
				spanMax[i] = xInt;
		}

		// segment 2-3 intersection
		if ((p[2].y <= i && p[3].y > i) ||
			(p[3].y <= i && p[2].y > i))
		{
			if (p[2].x == p[3].x)
				xInt = (int) p[2].x;
			else{
				m = (double)((p[2].y - p[3].y)/(p[2].x - p[3].x));
				xInt = (int64_t)(p[3].x + (double(i)-p[3].y)/m + 1.0F);
			}
			if (xInt < spanMin[i])
				spanMin[i] = xInt;
			if (xInt > spanMax[i])
				spanMax[i] = xInt;
		}

		// segment 3-0 intersection
		if ((p[3].y <= i && p[0].y > i) ||
			(p[0].y <= i && p[3].y > i))
		{
			if (p[3].x == p[0].x)   // vertical line
				xInt = (int) p[0].x;
			else{
				m = (double)((p[3].y - p[0].y)/(p[3].x - p[0].x));
				xInt = (int64_t)(p[0].x + (double(i)-p[0].y)/m + 1.0F);
			}
			if (xInt < spanMin[i])
				spanMin[i] = xInt;
			if (xInt > spanMax[i])
				spanMax[i] = xInt;
		}
	}

	TransformPoint(dTex, origin, -fAngle, fScale);     // This is the texture walking vector

	int64_t      strideDiv2    = mSurfaceArea.right;
	int64_t      strideSrcDiv2 = pSrc->GetArea().Width();
	uint32_t*     pDestBits     = (uint32_t*)mpPixels+rDstActual.top*strideDiv2; //Points to left side of destination buffer at the current row.
	uint32_t*     pSrcBits      = (uint32_t*)pSrc->GetPixels();
	uint32_t*     pCurrentDestBits;
	uint32_t      nCurrentColor;
	ZFPoint		tP;

	for(i=0; i<numRows; i++)
	{
		tP.x = spanMin[i] + (rDstActual.left-rDst.left) - destCenter.x + center.x; //Find first texture pixel in the source buffer. We do this by
		tP.y = (rDstActual.top + i) - rDst.top  - destCenter.y + center.y; //first taking the position of this pixel in the destination
		TransformPoint(tP, center, -fAngle, fScale);                                      //buffer and then translating and rotating it back to its 
		pCurrentDestBits = pDestBits+rDstActual.left+spanMin[i];               //position in the source buffer.

		tP.x += 0.0015f; //This is rather important because there is a slight chance that due to rounding errors
		tP.y += 0.0015f; // the texel value will be just slightly below zero. During integer rounding, the value
		// would round from -0.1 down to -1. This would be bad and cause crashes. 
		//Without these checks, you get rare crashes. Trust me, it happened. 

		for(int64_t lx=spanMin[i], lxEnd=spanMax[i]; lx<lxEnd; lx++)
		{
			nCurrentColor = *(pSrcBits + ((int64_t) tP.x) + ((int64_t)tP.y)*strideSrcDiv2);
			//*pCurrentDestBits = AlphaBlend_Col2Alpha(nCurrentColor, *pCurrentDestBits, 1.0f);
			*pCurrentDestBits = nCurrentColor;
			tP.x += dTex.x;
			tP.y += dTex.y;
			pCurrentDestBits++;
		}
		pDestBits += strideDiv2;
	}

	delete[] spanMinHeap;
	delete[] spanMaxHeap;

	return true;

//#define M_PI 3.141592653
}



bool ZBuffer::BltScaled(ZBuffer* pSrc)
{
    //const uint32_t* srcBuffer, int srcWidth, int srcHeight, uint32_t* destBuffer, int destWidth, int destHeight
    uint32_t* srcBuffer = pSrc->mpPixels;
    int64_t srcWidth = pSrc->GetArea().Width();
    int64_t srcHeight = pSrc->GetArea().Height();
    uint32_t* destBuffer = mpPixels;
    int64_t destWidth = mSurfaceArea.right;
    int64_t destHeight = mSurfaceArea.bottom;

    double xScale = static_cast<double>(srcWidth) / destWidth;
    double yScale = static_cast<double>(srcHeight) / destHeight;

    
    double fMaxRadius = sqrt(xScale * xScale + yScale * yScale);

    // if scaling up, we need to look at inverse radius
    if (fMaxRadius < 1.0)
        fMaxRadius = 1.0 / fMaxRadius;

    for (int y = 0; y < destHeight; ++y) 
    {
        for (int x = 0; x < destWidth; ++x) 
        {
            double srcX = x * xScale;
            double srcY = y * yScale;

            int srcXInt = static_cast<int>(srcX);
            int srcYInt = static_cast<int>(srcY);

            double r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            double weightSum = 0.0f;

            for (int32_t j = (int32_t)(srcYInt - yScale/2); j <= (int32_t)(srcYInt + yScale/2); j++)
            {
                for (int32_t i = (int32_t)(srcXInt - xScale/2); i <= (int32_t)(srcXInt + xScale/2); i++)
                {

                    double xDiff = std::abs(srcX - i);
                    double yDiff = std::abs(srcY - j);
                    double fDist = (sqrt(xDiff * xDiff + yDiff * yDiff));

                    if (i >= 0 && i < srcWidth && j >= 0 && j < srcHeight && fDist <= fMaxRadius) 
                    {
                        double weight = (fMaxRadius - fDist)/ fMaxRadius;

                        uint32_t pixel = srcBuffer[j * srcWidth + i];
                        double pixelA = static_cast<double>((pixel >> 24) & 0xFF);
                        double pixelR = static_cast<double>((pixel >> 16) & 0xFF);
                        double pixelG = static_cast<double>((pixel >> 8) & 0xFF);
                        double pixelB = static_cast<double>(pixel & 0xFF);

                        a += weight * pixelA;
                        r += weight * pixelR;
                        g += weight * pixelG;
                        b += weight * pixelB;

                        weightSum += weight;
                    }
                }
            }

            if (weightSum > 0.0f) 
            {
                a /= weightSum;
                r /= weightSum;
                g /= weightSum;
                b /= weightSum;
            }

            uint32_t destPixel = ((static_cast<uint32_t>(a) & 0xFF) << 24) |
                ((static_cast<uint32_t>(r) & 0xFF) << 16) |
                ((static_cast<uint32_t>(g) & 0xFF) << 8) |
                (static_cast<uint32_t>(b) & 0xFF);

            destBuffer[y * destWidth + x] = destPixel;
        }
    }

    return true;
}


void ZBuffer::Blur(float sigma, ZRect* pRect)
{
    ZBuffer temp(this);

    ZRect rArea(mSurfaceArea);
    if (pRect)
        rArea = *pRect;

    int64_t w = rArea.Width();
    int64_t h = rArea.Height();

    // Calculate the size of the kernel based on sigma (standard deviation)
    int64_t kernelSize = int64_t(6 * sigma) + 1;
    if (kernelSize % 2 == 0) 
    {
        kernelSize++; // Ensure an odd-sized kernel for symmetry
    }

    // Calculate the radius of the kernel
    int64_t kernelRadius = kernelSize / 2;

    // Create a 1D Gaussian kernel
    std::vector<float> kernel(kernelSize);
    float sum = 0.0f;
    for (int64_t i = 0; i < kernelSize; i++)
    {
        int64_t x = i - kernelRadius;
        kernel[i] = expf(-(x * x) / (2 * sigma * sigma));
        sum += kernel[i];
    }

    // Normalize the kernel
    for (int64_t i = 0; i < kernelSize; i++)
    {
        kernel[i] /= sum;
    }

    // Perform horizontal convolution
    for (int64_t y = rArea.top; y < rArea.bottom; y++)
    {
        for (int64_t x = rArea.left; x < rArea.right; x++)
        {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

            for (int64_t i = -kernelRadius; i <= kernelRadius; i++)
            {
                int64_t newX = x + i;
                if (newX < 0) 
                {
                    newX = 0;
                }
                else if (newX >= w) 
                {
                    newX = w - 1;
                }

                uint32_t pixel = mpPixels[y * w + newX];
                float weight = kernel[i + kernelRadius];

                r += ((pixel >> 16) & 0xFF) * weight;
                g += ((pixel >> 8) & 0xFF) * weight;
                b += (pixel & 0xFF) * weight;
                a += ((pixel >> 24) & 0xFF) * weight;
            }

            temp.mpPixels[y * w + x] = ((uint32_t(a) & 0xFF) << 24) |
                ((uint32_t(r) & 0xFF) << 16) |
                ((uint32_t(g) & 0xFF) << 8) |
                (uint32_t(b) & 0xFF);
        }
    }

    // Perform vertical convolution and store the result in the output buffer
    for (int64_t x = 0; x < w; x++)
    {
        for (int64_t y = 0; y < h; y++)
        {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

            for (int64_t i = -kernelRadius; i <= kernelRadius; i++)
            {
                int64_t newY = y + i;
                if (newY < 0)
                    newY = 0;
                else if (newY >= h) 
                    newY = h - 1;

                uint32_t pixel = temp.mpPixels[newY * w + x];
                float weight = kernel[i + kernelRadius];

                r += ((pixel >> 16) & 0xFF) * weight;
                g += ((pixel >> 8) & 0xFF) * weight;
                b += (pixel & 0xFF) * weight;
                a += ((pixel >> 24) & 0xFF) * weight;
            }

            mpPixels[y * w + x] = ((uint32_t(a) & 0xFF) << 24) |
                ((uint32_t(r) & 0xFF) << 16) |
                ((uint32_t(g) & 0xFF) << 8) |
                (uint32_t(b) & 0xFF);
        }
    }
}
