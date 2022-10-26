#include "ZImageWin.h"
#include "ZXMLNode.h"
#include "ZAnimator.h"
#include "ZStringHelpers.h"
#include "ZRasterizer.h"
#include "ZGraphicSystem.h"

extern ZAnimator gAnimator;

ZImageWin::ZImageWin()
{
	mbAcceptsCursorMessages = true;
    mpImage = nullptr;
	mfZoom = 1.0;
    mfPerfectFitZoom = 1.0;
    mfMinZoom = 0.01;
    mfMaxZoom = 1000.0;


    mbShowZoom = false;
    mbShow100Also = false;
    mZoomCaptionFontID = 0;
    mZoomCaptionColor = 0;
    mZoomCaptionPos = ZFont::kTopLeft;
}

ZImageWin::~ZImageWin()
{
}

bool ZImageWin::Init()
{
    mIdleSleepMS = 10000;
    return ZWin::Init();
}

bool ZImageWin::OnMouseUpL(int64_t x, int64_t y)
{
    ReleaseCapture();
    return ZWin::OnMouseUpL(x, y);
}

bool ZImageWin::OnMouseDownL(int64_t x, int64_t y)
{
    if (!msOnClickMessage.empty())
        gMessageSystem.Post(msOnClickMessage);


    if (mbZoomable && mImageArea.PtInRect(x,y))
    {
        if (SetCapture())
        {
//            OutputDebugLockless("capture x:%d, y:%d\n", mZoomOffset.mX, mZoomOffset.mY);
            SetMouseDownPos(mImageArea.left-x, mImageArea.top-y);
//            mZoomOffsetAtMouseDown = mZoomOffset;
            return true;
        }
    }

	return ZWin::OnMouseDownL(x, y);
}

bool ZImageWin::OnMouseDownR(int64_t x, int64_t y)
{
    FitImageToWindow();
    return ZWin::OnMouseDownR(x, y);
}


bool ZImageWin::OnMouseMove(int64_t x, int64_t y)
{
    // x,y in window space

    if (AmCapturing())
    {
        int32_t dX = (int32_t) x+mMouseDownOffset.mX;
        int32_t dY = (int32_t) y+mMouseDownOffset.mY;
        ScrollTo(dX, dY);
    }

    return ZWin::OnMouseMove(x, y);
}


bool ZImageWin::OnMouseWheel(int64_t x, int64_t y, int64_t nDelta)
{
    if (mbZoomable && mImageArea.PtInRect(x, y))
    {
        double fNewZoom = mfZoom;
        if (nDelta < 0)
        {
            fNewZoom *= 1.2f;
        }
        else
        {
            fNewZoom *= 0.8f;
        }

        // snap zoom to some specific values like perfect fit, 1.00, 2.00, 4.00
        if ((mfZoom > mfPerfectFitZoom && fNewZoom < mfPerfectFitZoom) || (mfZoom < mfPerfectFitZoom && fNewZoom > mfPerfectFitZoom))
            fNewZoom = mfPerfectFitZoom;
        else if ((mfZoom < 1.00 && fNewZoom > 1.00) || (mfZoom > 1.00 && fNewZoom < 1.00))
            fNewZoom = 1.00;
        else if ((mfZoom < 2.00 && fNewZoom > 2.00) || (mfZoom > 2.00 && fNewZoom < 2.00))
            fNewZoom = 2.00;
        else if ((mfZoom < 4.00 && fNewZoom > 4.00) || (mfZoom > 4.00 && fNewZoom < 4.00))
            fNewZoom = 4.00;

        SetZoom(fNewZoom);

        ZRect rImage(mpImage->GetArea());

        double fZoomPointX = x-mImageArea.left;
        double fZoomPointY = y-mImageArea.top;

        double fZoomPointU = fZoomPointX / (double)mImageArea.Width();
        double fZoomPointV = fZoomPointY / (double)mImageArea.Height();

        double fNewWidth = rImage.Width() * fNewZoom;
        double fNewHeight = rImage.Height() * fNewZoom;


        double fNewLeft = x - (fNewWidth * fZoomPointU);
        double fNewRight = fNewLeft + fNewWidth;

        double fNewTop = y -(fNewHeight * fZoomPointV);
        double fNewBottom = fNewTop + fNewHeight;

        ZRect rNewArea(fNewLeft, fNewTop, fNewRight, fNewBottom);

        mImageArea.SetRect(rNewArea);
	}

	return true;
}

void ZImageWin::ScrollTo(int32_t nX, int32_t nY)
{
    const int32_t kSnapDistance = 10;
    if (abs(nX - mAreaToDrawTo.left) < kSnapDistance)
    {
        nX = mAreaToDrawTo.left;    // snap left
    }
    else if (abs(nX + mImageArea.Width() - mAreaToDrawTo.right) < kSnapDistance)
    {
        nX = mAreaToDrawTo.right - mImageArea.Width(); // snap right
    }

    if (abs(nY - mAreaToDrawTo.top) < kSnapDistance)
    {
        nY = mAreaToDrawTo.top;    // snap top
    }
    else if (abs(nY + mImageArea.Height() - mAreaToDrawTo.bottom) < kSnapDistance)
    {
        nY = mAreaToDrawTo.bottom - mImageArea.Height(); // snap right
    }

    mImageArea.MoveRect(nX, nY);
    Invalidate();
}

void ZImageWin::LoadImage(const string& sName)
{
    delete mpImage;
    mpImage = new ZBuffer();
    mpImage->LoadBuffer(sName);

    FitImageToWindow();
}

void ZImageWin::FitImageToWindow()
{
    // determine initial scale to fit into window

    ZRect rImageArea(mpImage->GetArea());

    double fRatio = (double)rImageArea.Height() / (double)rImageArea.Width();

    int64_t nImageWidth = mArea.Width();
    int64_t nImageHeight = nImageWidth * fRatio;

    mfPerfectFitZoom = (double)nImageHeight / (double)rImageArea.Height();

    if (nImageHeight > mArea.Height())
    {
        nImageHeight = mArea.Height();
        nImageWidth = nImageHeight / fRatio;
        mfPerfectFitZoom = (double)nImageWidth / (double)rImageArea.Width();
    }

    int64_t nXOffset = (mArea.Width() - nImageWidth) / 2;
    int64_t nYOffset = (mArea.Height() - nImageHeight) / 2;


    mfZoom = mfPerfectFitZoom;
    mImageArea.SetRect(nXOffset, nYOffset, nXOffset + nImageWidth, nYOffset + nImageHeight);



    Invalidate();
}


void ZImageWin::SetZoom(double fZoom) 
{ 
    mfZoom = fZoom;
    if (mfZoom < mfMinZoom)
        mfZoom = mfMinZoom;
    else if (mfZoom > mfMaxZoom)
        mfZoom = mfMaxZoom;
    Invalidate(); 
}

double ZImageWin::GetZoom() 
{ 
    return mfZoom; 
}


void ZImageWin::SetImage(ZBuffer* pImage)
{
//	mpImage.reset(pImage);
    mpImage = pImage;
    FitImageToWindow();
}

bool ZImageWin::Paint()
{
    const std::lock_guard<std::mutex> surfaceLock(mpTransformTexture->GetMutex());
    if (!mbInvalid)
        return true;

    ZRect rDest(mpTransformTexture->GetArea());
    ZRect rSource(mpImage->GetArea());

    mpTransformTexture->Fill(mpTransformTexture->GetArea(), mFillColor);

    if (mfZoom == 1.0f && rDest == rSource)  // simple blt?
    {
        mpTransformTexture->Blt(mpImage, rSource, rDest);
    }
    else
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(mImageArea, verts);
        gRasterizer.RasterizeWithAlpha(mpTransformTexture, mpImage, verts, &mAreaToDrawTo);
    }

    if (!msCaption.empty())
    {
        std::shared_ptr<ZFont> pFont(gpFontSystem->GetDefaultFont(mCaptionFontID));
        assert(pFont);

        ZRect rCaption(pFont->GetOutputRect(mAreaToDrawTo, msCaption.data(), msCaption.length(), mCaptionPos));
        pFont->DrawText(mpTransformTexture, msCaption, rCaption, mnCaptionCol, mnCaptionCol);
    }

    if (mbShowZoom)
    {
        if ((mfZoom == 1.0 && mbShow100Also) || mfZoom != 1.0)
        {
            std::shared_ptr<ZFont> pFont(gpFontSystem->GetDefaultFont(mZoomCaptionFontID));
            assert(pFont);

            string sZoom;
            Sprintf(sZoom, "%d%%", (int32_t)(mfZoom * 100.0));

            ZRect rZoomCaption(pFont->GetOutputRect(mAreaToDrawTo, sZoom.data(), sZoom.length(), mZoomCaptionPos));
            pFont->DrawText(mpTransformTexture, sZoom, rZoomCaption, mZoomCaptionColor, mZoomCaptionColor);
        }
    }

	return ZWin::Paint();
}

bool ZImageWin::InitFromXML(ZXMLNode* pNode)
{
	string sName;

	sName = pNode->GetAttribute("name");
	LoadImage(sName);

	if (!ZWin::InitFromXML(pNode))
		return false;

	ZXMLNode* pTransform = pNode->GetChild("trans");
	if (pTransform)
	{
/*		cCEAnimObject_TransformingImage* pNewTransImage = new cCEAnimObject_TransformingImage(&mImage);
		pNewTransImage->DoTransformation(pTransform->GetText());

		gAnimator.AddObject(pNewTransImage, this);*/

		DoTransformation(pTransform->GetText());
	}
	else if (pNode->HasAttribute("pos"))
	{
		ZTransformation trans(StringToPoint(pNode->GetAttribute("pos")));
		SetTransform(trans);
	}

	return true;
}
