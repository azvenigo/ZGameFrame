#include "LifeWin.h"
#include "ZGraphicSystem.h"
#include "ZWinBtn.h"
#include "ZRasterizer.h"
#include "ZRandom.h"
#include "Resources.h"
extern bool gbApplicationExiting;

using namespace std;

cLifeWin::cLifeWin()
{
	mnWidth = 0;
	mnHeight = 0;
	mpGrid = NULL;
	mpGrid2 = NULL;
	mpCurGrid = NULL;
	mpFont = NULL;
}


const int64_t kSquareSize = 1;

const uint32_t kOnPixel = 0xffffffff;
const uint32_t kOffPixel = 0xff000000;

/////////////////////////////////////////////////////////////////////////
// 
// cLifeWin
//
/////////////////////////////////////////////////////////////////////////

bool cLifeWin::Init()
{
//	mpGrid  = new char[mnWidth * mnHeight];
//	mpGrid2 = new char[mnWidth * mnHeight];

	mpGrid.reset(new ZBuffer());
	mpGrid->Init(mnWidth, mnHeight);

	mpGrid2.reset(new ZBuffer());
	mpGrid2->Init(mnWidth, mnHeight);


	mpCurGrid = mpGrid;

    mnOnPixelCol = kOnPixel;
    //mnOffPixelCol = kOffPixel;

    mnOffPixelCol = RANDI64(0xff000000, 0xffffffff);

	mpFont = gpFontSystem->GetFont(gDefaultTextFont);


	mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;

    SetFocus();


//	SetSize(mnWidth*kSquareSize, mnHeight*kSquareSize+16);

	for (int64_t y = 0; y < mnHeight; y++)
		for (int64_t x = 0; x < mnWidth ; x++)
		{
			uint32_t nCol = mnOffPixelCol;
//			if (RANDBOOL)
            if (RANDPERCENT(30))
				nCol = mnOnPixelCol;

			mpGrid->SetPixel(x, y, nCol);
		}

	mTimer.Start();

	mnIterations = 0;
	mbPaused = false;
	mbForward = true;

//	PrivateBuffer(true);

	return ZWin::Init();
}

bool cLifeWin::Shutdown()
{
	if (!mbInitted)
		return false;

	ZWin::Shutdown();

	mpGrid = nullptr;
	mpGrid2 = nullptr;

/*	if (mpFont)
	{
		mpFont->Shutdown();
		delete mpFont;
		mpFont = NULL;
	}*/
//	PrivateBuffer(false);
	return true;
}


bool cLifeWin::Paint()
{
    if (!mbInvalid)
        return false;

//	if (gbApplicationExiting)
//		return true;

	const std::lock_guard<std::mutex> lock(mShutdownMutex);

	string sTemp;
	ZASSERT(mpSurface.get());


	ZRect rGrid(mAreaLocal.left, mAreaLocal.top, mAreaLocal.right, mAreaLocal.bottom - 16);
	ZRect rHandle(mAreaLocal.left, mAreaLocal.bottom-16, mAreaLocal.right, mAreaLocal.bottom);

//	mpTransformTexture->Fill(rGrid, mpTransformTexture->ConvertRGB(255, 128,128,128));
//	mpTransformTexture->Fill(rHandle, mpTransformTexture->ConvertRGB(255, 128,128,255));

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface->GetMutex());
    PaintGrid();

	Sprintf(sTemp, "I: %ld  C: %ld", mnIterations, mnNumCells);

	if (mbPaused)
		sTemp += " PAUSED";

	ZRect rText(0, 0, mAreaLocal.right, mAreaLocal.bottom);

//    TIME_SECTION_START(LifeWinDrawText);
    mpFont->DrawText(mpSurface.get(), sTemp, rText, &ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xffffff00, 0xffffff00));
//    TIME_SECTION_END(LifeWinDrawText);
    
	ZWin::Paint();

    mbInvalid = true;
    return true;
}

void cLifeWin::PaintGrid()
{
	ZRasterizer rasterizer;
	tUVVertexArray verts;
    rasterizer.RectToVerts(mAreaLocal, verts);

    rasterizer.Rasterize(mpSurface.get(), mpCurGrid.get(), verts);
}

bool cLifeWin::OnMouseDownL(int64_t x, int64_t y)
{
	double kPixelsPerGridX = (double) mAreaInParent.Width()/mnWidth;
	double kPixelsPerGridY = (double) mAreaInParent.Height()/mnHeight;


	int64_t nClickedGridX = (int64_t) (x/ kPixelsPerGridX);
	int64_t nClickedGridY = (int64_t) (y/ kPixelsPerGridY);

	//DEBUG_OUT("Clicked %ld, %ld\n", nClickedGridX, nClickedGridY);
	if (nClickedGridX < mnWidth && nClickedGridY < mnHeight)
	{
		SetCapture();
		uint32_t nSetPixel = mnOffPixelCol;
		if (mpCurGrid->GetPixel(nClickedGridX, nClickedGridY) == mnOffPixelCol)
			nSetPixel = mnOnPixelCol;

		mbPainting = true;
		mpCurGrid->SetPixel(nClickedGridX, nClickedGridY, nSetPixel);
	}
	else
		return ZWin::OnMouseDownL(x, y);

	return true;
}

bool cLifeWin::OnMouseUpL(int64_t x, int64_t y)
{
	mbPainting = false;
	ReleaseCapture();
	return ZWin::OnMouseUpL(x, y);
}

bool cLifeWin::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
	switch (key)
	{
	case ' ':
		mnIterations = 0;
		//memset(mpCurGrid, 0, mnWidth*mnHeight);
		mpCurGrid->Fill(mnOffPixelCol, &mpCurGrid->GetArea());
		break;

    case VK_ESCAPE:
        gMessageSystem.Post("quit_app_confirmed");
        break;
	case '+':
		if (mbPaused)
			Process();
		break;
	}
#endif
	return ZWin::OnKeyDown(key);
}



bool cLifeWin::OnMouseMove(int64_t x, int64_t y)
{
	if (AmCapturing() && mbPainting)
	{
		double kPixelsPerGridX = (double) mAreaInParent.Width() / (double) mnWidth;
		double kPixelsPerGridY = (double) mAreaInParent.Height() / (double) mnHeight;


		int64_t nClickedGridX = (int64_t) ((double) x / kPixelsPerGridX);
		int64_t nClickedGridY = (int64_t) ((double) y / kPixelsPerGridY);

		if (nClickedGridX < 0 || nClickedGridX >= mnWidth || 
			nClickedGridY < 0 || nClickedGridY >= mnHeight)
			return true;


		if (mbPainting)
			mpCurGrid->SetPixel(nClickedGridX, nClickedGridY, mnOnPixelCol);
		else
			mpCurGrid->SetPixel(nClickedGridX, nClickedGridY, mnOffPixelCol);
		return true;
	}

	return ZWin::OnMouseMove(x, y);
}


bool cLifeWin::OnMouseDownR(int64_t, int64_t)
{
	mbPaused = !mbPaused;
	return true;
}




bool cLifeWin::Process()
{
    if (mbPaused || mTimer.GetElapsedTime() < 0)
        return false;


    const std::lock_guard<std::mutex> lock(mShutdownMutex);

    mTimer.Reset();

    tZBufferPtr	pFromGrid;

	mnIterations ++;
	mnNumCells = 0;

	if (mbForward)
	{
		pFromGrid = mpGrid;
		mpCurGrid = mpGrid2;
	}
	else
	{
		pFromGrid = mpGrid2;
		mpCurGrid = mpGrid;
	}


	//memset(mpCurGrid, 0, mnWidth * mnHeight);
	mpCurGrid->Fill(mnOffPixelCol);

	for (int64_t y = 1; y < mnHeight-1; y++)
		for (int64_t x = 1; x < mnWidth-1; x++)
		{
			int64_t nSum = 0;
			for (int64_t j = -1; j <= 1; j++)
				for (int64_t i = -1; i <= 1; i++)
				{
					if (!(j == 0 && i == 0))
					{
						if (pFromGrid->GetPixel(x+i, y + j) == mnOnPixelCol)
							nSum++;
					}
				}

			if (pFromGrid->GetPixel(x, y) == mnOnPixelCol && (nSum == 2 || nSum == 3))
			{
				mpCurGrid->SetPixel(x, y, mnOnPixelCol);
				mnNumCells++;
			}
			if (pFromGrid->GetPixel(x, y) == mnOffPixelCol && nSum == 3)
			{
				mpCurGrid->SetPixel(x, y, mnOnPixelCol);
				mnNumCells++;
			}
		}

	mbForward = !mbForward;

    return true;
}


/*void cLifeWin::Load(cCEString sName)
{
	HANDLE hFile = ::CreateFile(sName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,	OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD nWidth = 0;
		DWORD nHeight = 0;
		DWORD nIterations = 0;
		DWORD nNumRead;

		::ReadFile(hFile, &nWidth, sizeof(DWORD), &nNumRead, NULL);
		::ReadFile(hFile, &nHeight, sizeof(DWORD), &nNumRead, NULL);
		::ReadFile(hFile, &nIterations, sizeof(DWORD), &nNumRead, NULL);

		if (nWidth == mnWidth && nHeight == mnHeight)
		{
			mnIterations = nIterations;
			::ReadFile(hFile, mpCurGrid, mnWidth * mnHeight, &nNumRead, NULL);
		}
		::CloseHandle(hFile);
	}
}

void cLifeWin::Save(cCEString sName)
{
	HANDLE hFile = ::CreateFile(sName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD nWidth = mnWidth;
		DWORD nHeight = mnHeight;
		DWORD nIterations = mnIterations;
		DWORD nNumWritten;

		::WriteFile(hFile, &nWidth, sizeof(DWORD), &nNumWritten, NULL);
		::WriteFile(hFile, &nHeight, sizeof(DWORD), &nNumWritten, NULL);
		::WriteFile(hFile, &nIterations, sizeof(DWORD), &nNumWritten, NULL);
		::WriteFile(hFile, mpCurGrid, mnWidth * mnHeight, &nNumWritten, NULL);
		::CloseHandle(hFile);
	}
}
*/