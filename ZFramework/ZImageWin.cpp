#include "ZImageWin.h"
#include "ZXMLNode.h"
#include "ZAnimator.h"
#include "ZStringHelpers.h"
#include "ZRasterizer.h"
#include "ZGraphicSystem.h"
#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "Resources.h"

extern ZAnimator gAnimator;
using namespace std;

ZImageWin::ZImageWin()
{
	mbAcceptsCursorMessages = true;
    mpImage.reset();
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

    if (mbZoomable || !msSelectMessage.empty())
    {
        ZWinControlPanel* pPanel = new ZWinControlPanel();
        ZRect rPanelArea(mAreaToDrawTo.left, mAreaToDrawTo.bottom - 16, mAreaToDrawTo.right, mAreaToDrawTo.bottom);
        pPanel->SetTriggerRect(mAreaAbsolute);
        pPanel->SetHideOnMouseExit(true);
        pPanel->SetArea(rPanelArea);
        ChildAdd(pPanel);

        if (mbZoomable)
        {
            ZRect rButton(0, 0, 16, 16);

            ZWinSizablePushBtn* pBtn;

            pBtn = new ZWinSizablePushBtn();
            pBtn->SetImages(gStandardButtonUpEdgeImage.get(), gStandardButtonDownEdgeImage.get(), grStandardButtonEdge);
            pBtn->SetCaption(":"); // wingdings rotate left
            pBtn->SetFont(gpFontSystem->GetDefaultFont("Wingdings 3", 12));
            pBtn->SetColor(0xffffffff);
            pBtn->SetColor2(0xffffffff);
            pBtn->SetStyle(ZFont::kNormal);

            pBtn->SetArea(rButton);


            string sMessage;
            Sprintf(sMessage, "type=rotate_left;target=%s", GetTargetName().c_str());
            pBtn->SetMessage(sMessage);
            pPanel->ChildAdd(pBtn);



            rButton.OffsetRect(16, 0);

            pBtn = new ZWinSizablePushBtn();
            pBtn->SetImages(gStandardButtonUpEdgeImage.get(), gStandardButtonDownEdgeImage.get(), grStandardButtonEdge);
            pBtn->SetCaption(";"); // wingdings rotate right
            pBtn->SetFont(gpFontSystem->GetDefaultFont("Wingdings 3", 12));
            pBtn->SetColor(0xffffffff);
            pBtn->SetColor2(0xffffffff);
            pBtn->SetStyle(ZFont::kNormal);

            pBtn->SetArea(rButton);

            Sprintf(sMessage, "type=rotate_right;target=%s", GetTargetName().c_str());

            pBtn->SetMessage(sMessage);


            pPanel->ChildAdd(pBtn);
        }

        if (!msSelectMessage.empty())
        {
            ZWinSizablePushBtn* pBtn;

            ZRect rButton(rPanelArea.Width() - 64,0, rPanelArea.Width(), rPanelArea.Height());
            rButton.left = rPanelArea.right - 64;

            pBtn = new ZWinSizablePushBtn();
            pBtn->SetImages(gStandardButtonUpEdgeImage.get(), gStandardButtonDownEdgeImage.get(), grStandardButtonEdge);
            pBtn->SetCaption("set"); // wingdings rotate left
            pBtn->SetFont(gpFontSystem->GetDefaultFont(0));
            pBtn->SetColor(0xffffffff);
            pBtn->SetColor2(0xffffffff);
            pBtn->SetStyle(ZFont::kNormal);

            pBtn->SetArea(rButton);

            pBtn->SetMessage(msSelectMessage);
            pPanel->ChildAdd(pBtn);
        }

    }




    return ZWin::Init();
}

bool ZImageWin::OnMouseUpL(int64_t x, int64_t y)
{
    ReleaseCapture();
    return ZWin::OnMouseUpL(x, y);
}

bool ZImageWin::OnMouseDownL(int64_t x, int64_t y)
{
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
        int32_t dX = (int32_t) (x+mMouseDownOffset.mX);
        int32_t dY = (int32_t) (y+mMouseDownOffset.mY);
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

        ZRect rImage(mpImage.get()->GetArea());

        double fZoomPointX = (double)(x-mImageArea.left);
        double fZoomPointY = (double)(y-mImageArea.top);

        double fZoomPointU = fZoomPointX / (double)mImageArea.Width();
        double fZoomPointV = fZoomPointY / (double)mImageArea.Height();

        double fNewWidth = rImage.Width() * fNewZoom;
        double fNewHeight = rImage.Height() * fNewZoom;


        int64_t nNewLeft = (int64_t)(x - (fNewWidth * fZoomPointU));
        int64_t nNewRight = nNewLeft + (int64_t)fNewWidth;

        int64_t nNewTop = (int64_t)(y - (fNewHeight * fZoomPointV));
        int64_t nNewBottom = nNewTop + (int64_t)fNewHeight;

        ZRect rNewArea(nNewLeft, nNewTop, nNewRight, nNewBottom);

        mImageArea.SetRect(rNewArea);
	}

	return true;
}

void ZImageWin::ScrollTo(int64_t nX, int64_t nY)
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
    mpImage.reset(new ZBuffer());
    mpImage->LoadBuffer(sName);

    FitImageToWindow();
}

void ZImageWin::FitImageToWindow()
{
    // determine initial scale to fit into window

    ZRect rImageArea(mpImage->GetArea());

    double fRatio = (double)rImageArea.Height() / (double)rImageArea.Width();

    int64_t nImageWidth = mArea.Width();
    int64_t nImageHeight = (int64_t)(nImageWidth * fRatio);

    mfPerfectFitZoom = (double)nImageHeight / (double)rImageArea.Height();

    if (nImageHeight > mArea.Height())
    {
        nImageHeight = mArea.Height();
        nImageWidth = (int64_t) (nImageHeight / fRatio);
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


void ZImageWin::SetImage(tZBufferPtr pImage)
{
//	mpImage.reset(pImage);
    mpImage = pImage;
    FitImageToWindow();
}

bool ZImageWin::Paint()
{
    const std::lock_guard<std::mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());
    
    if (!mbInvalid)
        return true;
    const std::lock_guard<std::mutex> imageSurfaceLock(mpImage.get()->GetMutex());

    ZRect rDest(mpTransformTexture.get()->GetArea());
    ZRect rSource(mpImage->GetArea());

    mpTransformTexture.get()->Fill(mpTransformTexture.get()->GetArea(), mFillColor);

    if (mfZoom == 1.0f && rDest == rSource)  // simple blt?
    {
        mpTransformTexture.get()->Blt(mpImage.get(), rSource, rDest);
    }
    else
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(mImageArea, verts);
        gRasterizer.RasterizeWithAlpha(mpTransformTexture.get(), mpImage.get(), verts, &mAreaToDrawTo);
    }

    if (!msCaption.empty())
    {
        tZFontPtr pFont(gpFontSystem->GetDefaultFont(mCaptionFontID));
        assert(pFont);

        ZRect rCaption(pFont->GetOutputRect(mAreaToDrawTo, msCaption.data(), msCaption.length(), mCaptionPos));
        pFont->DrawText(mpTransformTexture.get(), msCaption, rCaption, mnCaptionCol, mnCaptionCol);
    }

    if (mbShowZoom)
    {
        if ((mfZoom == 1.0 && mbShow100Also) || mfZoom != 1.0)
        {
            tZFontPtr pFont(gpFontSystem->GetDefaultFont(mZoomCaptionFontID));
            assert(pFont);

            string sZoom;
            Sprintf(sZoom, "%d%%", (int32_t)(mfZoom * 100.0));

            ZRect rZoomCaption(pFont->GetOutputRect(mAreaToDrawTo, sZoom.data(), sZoom.length(), mZoomCaptionPos));
            pFont->DrawText(mpTransformTexture.get(), sZoom, rZoomCaption, mZoomCaptionColor, mZoomCaptionColor);
        }
    }

	return ZWin::Paint();
}

bool ZImageWin::Rotate(eRotation rotation)
{
    if (rotation == kLeft)
    {
        tZBufferPtr pBuf(new ZBuffer());

        ZRect rOldArea(mpImage->GetArea());
        ZRect rNewArea(0, 0, rOldArea.Height(), rOldArea.Width());

        pBuf->Init(rNewArea.Width(), rNewArea.Height());
        for (int y = 0; y < rOldArea.Height(); y++)
        {
            for (int x = 0; x < rOldArea.Width(); x++)
            {
                pBuf->SetPixel(y, rNewArea.Height()-x-1, mpImage->GetPixel(x, y));
            }
        }

        mpImage->Init(rNewArea.Width(), rNewArea.Height());
        mpImage->Blt(pBuf.get(), rNewArea, rNewArea);

//        mpImage = pBuf;
    }
    else if (rotation == kRight)
    {
        tZBufferPtr pBuf(new ZBuffer());

        ZRect rOldArea(mpImage->GetArea());
        ZRect rNewArea(0, 0, rOldArea.Height(), rOldArea.Width());

        pBuf->Init(rNewArea.Width(), rNewArea.Height());
        for (int y = 0; y < rOldArea.Height(); y++)
        {
            for (int x = 0; x < rOldArea.Width(); x++)
            {
                pBuf->SetPixel(rNewArea.Width()-y-1, x, mpImage->GetPixel(x, y));
            }
        }

        mpImage->Init(rNewArea.Width(), rNewArea.Height());
        mpImage->Blt(pBuf.get(), rNewArea, rNewArea);
    }

    FitImageToWindow();
    return true;
}

bool ZImageWin::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "rotate_left")
    {
        return Rotate(kLeft);
    }
    else if (sType == "rotate_right")
    {
        return Rotate(kRight);
    }

    return ZWin::HandleMessage(message);
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
