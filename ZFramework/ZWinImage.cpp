#include "ZWinImage.h"
#include "ZXMLNode.h"
#include "ZAnimator.h"
#include "ZStringHelpers.h"
#include "ZRasterizer.h"
#include "ZGraphicSystem.h"
#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "Resources.h"
#include "ZDebug.h"

extern ZAnimator gAnimator;
using namespace std;

ZWinImage::ZWinImage()
{
	mbAcceptsCursorMessages = true;
    mpImage.reset();
    mbZoomable = false;
    mbControlPanelEnabled = false;
    mnControlPanelButtonSide = 0;
    mnControlPanelFontHeight = 0;
	mfZoom = 1.0;
    mfPerfectFitZoom = 1.0;
    mfMinZoom = 0.01;
    mfMaxZoom = 1000.0;


    mbShowZoom = false;
    mbShow100Also = false;
    mZoomCaptionLook = ZTextLook(ZTextLook::kNormal, 0xff000000, 0xff000000);
    mZoomCaptionPos = ZGUI::LT;
    mpPanel = nullptr;
}

ZWinImage::~ZWinImage()
{
}

bool ZWinImage::Init()
{
    mIdleSleepMS = 10000;

    if (mbControlPanelEnabled)
    {
        mpPanel = new ZWinControlPanel();
        mnControlPanelButtonSide = mAreaToDrawTo.Height() / 16;
        mnControlPanelFontHeight = mAreaToDrawTo.Height() / 24;
        if (mnControlPanelButtonSide < 20)
            mnControlPanelButtonSide = 20;
        if (mnControlPanelFontHeight < 16)
            mnControlPanelFontHeight = 16;

        ZRect rPanelArea(mAreaToDrawTo.left, mAreaToDrawTo.bottom - mnControlPanelButtonSide, mAreaToDrawTo.right, mAreaToDrawTo.bottom);
        mpPanel->SetTriggerRect(mAreaAbsolute);
        mpPanel->SetHideOnMouseExit(true);
        mpPanel->SetArea(rPanelArea);
        ChildAdd(mpPanel);

        ZRect rButton(0, 0, mnControlPanelButtonSide, mnControlPanelButtonSide);

        ZWinSizablePushBtn* pBtn;

        if (!msCloseButtonMessage.empty())
        {

            pBtn = new ZWinSizablePushBtn();
            pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
            pBtn->SetCaption("X"); 
            pBtn->SetArea(rButton);
            pBtn->SetMessage(msCloseButtonMessage);
            mpPanel->ChildAdd(pBtn);

            rButton.OffsetRect(mnControlPanelButtonSide *2, 0);
        }






        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption(":"); // wingdings rotate left
        pBtn->mStyle = ZGUI::Style(ZFontParams("Wingdings 3", mnControlPanelFontHeight));
        pBtn->SetArea(rButton);


        string sMessage;
        Sprintf(sMessage, "rotate_left;target=%s", GetTargetName().c_str());
        pBtn->SetMessage(sMessage);
        mpPanel->ChildAdd(pBtn);



        rButton.OffsetRect(mnControlPanelButtonSide, 0);

        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption(";"); // wingdings rotate right
        pBtn->mStyle = ZGUI::Style(ZFontParams("Wingdings 3", mnControlPanelFontHeight));
        pBtn->SetArea(rButton);

        Sprintf(sMessage, "rotate_right;target=%s", GetTargetName().c_str());

        pBtn->SetMessage(sMessage);
        mpPanel->ChildAdd(pBtn);

        rButton.OffsetRect(mnControlPanelButtonSide, 0);

        if (!msSaveButtonMessage.empty())
        {
            rButton.right = rButton.left + mnControlPanelButtonSide *3;

            pBtn = new ZWinSizablePushBtn();
            pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
            pBtn->SetCaption("save"); 
            pBtn->SetArea(rButton);
            pBtn->SetMessage(msSaveButtonMessage);
            mpPanel->ChildAdd(pBtn);
        }
    }




    return ZWin::Init();
}

bool ZWinImage::OnMouseUpL(int64_t x, int64_t y)
{
    ReleaseCapture();
    if (!msMouseUpLMessage.empty() && mImageArea.PtInRect(x, y))
        gMessageSystem.Post(msMouseUpLMessage);
    return ZWin::OnMouseUpL(x, y);
}

bool ZWinImage::OnMouseDownL(int64_t x, int64_t y)
{
    if (mbZoomable && mImageArea.PtInRect(x,y))
    {
        if (SetCapture())
        {
//            OutputDebugLockless("capture x:%d, y:%d\n", mZoomOffset.x, mZoomOffset.y);
            SetMouseDownPos(mImageArea.left-x, mImageArea.top-y);
//            mZoomOffsetAtMouseDown = mZoomOffset;
            return true;
        }
    }

	return ZWin::OnMouseDownL(x, y);
}

bool ZWinImage::OnMouseDownR(int64_t x, int64_t y)
{
    FitImageToWindow();
    return ZWin::OnMouseDownR(x, y);
}


bool ZWinImage::OnMouseMove(int64_t x, int64_t y)
{
    // x,y in window space

    if (AmCapturing())
    {
        int32_t dX = (int32_t) (x+mMouseDownOffset.x);
        int32_t dY = (int32_t) (y+mMouseDownOffset.y);
        ScrollTo(dX, dY);
    }

    return ZWin::OnMouseMove(x, y);
}


bool ZWinImage::OnMouseWheel(int64_t x, int64_t y, int64_t nDelta)
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

void ZWinImage::ScrollTo(int64_t nX, int64_t nY)
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

void ZWinImage::LoadImage(const string& sName)
{
    mbVisible = false;
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    // if there's an old image, acquire the lock before freeing it
    mpImage.reset(new ZBuffer());

    const std::lock_guard<std::recursive_mutex> imageSurfaceLock(mpImage.get()->GetMutex());
    mpImage->LoadBuffer(sName);

    FitImageToWindow();
    mbVisible = true;
}

void ZWinImage::SetArea(const ZRect& newArea)
{
    mbVisible = false;
    ZWin::SetArea(newArea);

    if (mpPanel)
    {
        ZRect rPanelArea(mAreaToDrawTo.left, mAreaToDrawTo.bottom - mnControlPanelButtonSide, mAreaToDrawTo.right, mAreaToDrawTo.bottom);
        mpPanel->SetTriggerRect(mAreaAbsolute);
        mpPanel->SetArea(rPanelArea);
    }

    FitImageToWindow();
    mbVisible = true;
}


void ZWinImage::FitImageToWindow()
{
    if (!mpImage)
        return;

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


void ZWinImage::SetZoom(double fZoom) 
{ 
    mfZoom = fZoom;
    if (mfZoom < mfMinZoom)
        mfZoom = mfMinZoom;
    else if (mfZoom > mfMaxZoom)
        mfZoom = mfMaxZoom;
    Invalidate(); 
}

double ZWinImage::GetZoom() 
{ 
    return mfZoom; 
}


void ZWinImage::SetImage(tZBufferPtr pImage)
{
//	mpImage.reset(pImage);
    mpImage = pImage;
    FitImageToWindow();
}

bool ZWinImage::Paint()
{
    if (!mpTransformTexture)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    if (!mbVisible)
        return false;

    if (!mbInvalid)
        return false;


    ZASSERT(mpTransformTexture.get()->GetPixels() != nullptr);
    ZOUT_LOCKLESS("painting ", msWinName);

    ZRect rDest(mpTransformTexture.get()->GetArea());

    mpTransformTexture.get()->Fill(mpTransformTexture.get()->GetArea(), mFillColor);
    ZASSERT(mpTransformTexture.get()->GetPixels() != nullptr);


    if (mpImage)
    {
        const std::lock_guard<std::recursive_mutex> imageSurfaceLock(mpImage.get()->GetMutex());
        ZRect rSource(mpImage->GetArea());

        if (mfZoom == 1.0f && rDest == rSource)  // simple blt?
        {
            mpTransformTexture.get()->Blt(mpImage.get(), rSource, rDest);
        }
        else
        {
            tUVVertexArray verts;
            gRasterizer.RectToVerts(mImageArea, verts);
            ZASSERT(mpTransformTexture.get()->GetPixels() != nullptr);
            gRasterizer.RasterizeWithAlpha(mpTransformTexture.get(), mpImage.get(), verts, &mAreaToDrawTo);
        }
    }

    if (!msCaption.empty())
    {
        assert(mpCaptionFont);

        ZRect rCaption(mpCaptionFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)msCaption.data(), msCaption.length(), mCaptionPos));
        mpCaptionFont->DrawText(mpTransformTexture.get(), msCaption, rCaption, mCaptionLook);
    }

    if (mbShowZoom)
    {
        if ((mfZoom == 1.0 && mbShow100Also) || mfZoom != 1.0)
        {
            assert(mpZoomCaptionFont);

            string sZoom;
            Sprintf(sZoom, "%d%%", (int32_t)(mfZoom * 100.0));

            ZRect rZoomCaption(mpZoomCaptionFont->GetOutputRect(mAreaToDrawTo, (uint8_t*)sZoom.data(), sZoom.length(), mZoomCaptionPos));
            mpZoomCaptionFont->DrawText(mpTransformTexture.get(), sZoom, rZoomCaption, mZoomCaptionLook);
        }
    }

	return ZWin::Paint();
}

bool ZWinImage::Rotate(eRotation rotation)
{
    if (rotation == kLeft)
    {
        ZBuffer buf;

        ZRect rOldArea(mpImage->GetArea());
        ZRect rNewArea(0, 0, rOldArea.Height(), rOldArea.Width());

        buf.Init(rNewArea.Width(), rNewArea.Height());
        for (int y = 0; y < rOldArea.Height(); y++)
        {
            for (int x = 0; x < rOldArea.Width(); x++)
            {
                buf.SetPixel(y, rNewArea.Height()-x-1, mpImage->GetPixel(x, y));
            }
        }

        mpImage->Init(rNewArea.Width(), rNewArea.Height());
        mpImage->Blt(&buf, rNewArea, rNewArea);

//        mpImage = pBuf;
    }
    else if (rotation == kRight)
    {
        ZBuffer buf;

        ZRect rOldArea(mpImage->GetArea());
        ZRect rNewArea(0, 0, rOldArea.Height(), rOldArea.Width());

        buf.Init(rNewArea.Width(), rNewArea.Height());
        for (int y = 0; y < rOldArea.Height(); y++)
        {
            for (int x = 0; x < rOldArea.Width(); x++)
            {
                buf.SetPixel(rNewArea.Width()-y-1, x, mpImage->GetPixel(x, y));
            }
        }

        mpImage->Init(rNewArea.Width(), rNewArea.Height());
        mpImage->Blt(&buf, rNewArea, rNewArea);
    }

    FitImageToWindow();
    return true;
}

bool ZWinImage::HandleMessage(const ZMessage& message)
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


bool ZWinImage::InitFromXML(ZXMLNode* pNode)
{
	string sName;

	sName = pNode->GetAttribute("name");
	LoadImage(sName);

	if (!ZWin::InitFromXML(pNode))
		return false;

	ZXMLNode* pTransform = pNode->GetChild("trans");
	if (pTransform)
	{
		DoTransformation(pTransform->GetText());
	}
	else if (pNode->HasAttribute("pos"))
	{
		ZTransformation trans(StringToPoint(pNode->GetAttribute("pos")));
		SetTransform(trans);
	}

	return true;
}
