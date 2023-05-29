#include "ZWinImage.h"
#include "ZXMLNode.h"
#include "ZAnimator.h"
#include "ZStringHelpers.h"
#include "ZRasterizer.h"
#include "ZGraphicSystem.h"
#include "ZWinControlPanel.h"
#include "ZWinBtn.H"
#include "Resources.h"
#include "ZGUIStyle.h"
#include "ZDebug.h"

extern ZAnimator gAnimator;
using namespace std;

ZWinImage::ZWinImage()
{
	mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mpImage.reset();
    mnControlPanelButtonSide = 0;
	mfZoom = 1.0;
    mfPerfectFitZoom = 1.0;
    mViewState = kFitToWindow;
    mfMinZoom = 0.01;
    mfMaxZoom = 1000.0;
    mManipulationHotkey = 0;
    mbHotkeyActive = false;
    mZoomStyle = gStyleCaption;

    mpPanel = nullptr;

    mBehavior = eBehavior::kNone;
}

ZWinImage::~ZWinImage()
{
}

bool ZWinImage::Init()
{
    mIdleSleepMS = 10000;

    if (mBehavior & eBehavior::kShowControlPanel)
    {
        mpPanel = new ZWinControlPanel();
        mnControlPanelButtonSide = mAreaToDrawTo.Height() / 16;

        limit<int64_t>(mnControlPanelButtonSide, 20, 64);

        ZGUI::Style wd1Style = ZGUI::Style(ZFontParams("Wingdings", mnControlPanelButtonSide/2));
        ZGUI::Style wd3Style = ZGUI::Style(ZFontParams("Wingdings 3", mnControlPanelButtonSide*2/3));


        ZRect rPanelArea(mAreaToDrawTo.left, mAreaToDrawTo.top, mAreaToDrawTo.right, mAreaToDrawTo.top + mnControlPanelButtonSide + 2* gDefaultSpacer);
        mpPanel->SetTriggerRect(mAreaAbsolute);
        //mpPanel->SetHideOnMouseExit(true);
        mpPanel->SetArea(rPanelArea);
        ChildAdd(mpPanel, (mBehavior&kShowOnHotkey == 0));

        ZWinSizablePushBtn* pBtn;
        ZRect rButton(gDefaultSpacer, gDefaultSpacer, mnControlPanelButtonSide, mnControlPanelButtonSide);



        ZRect rGroup(rButton);  // start the first grouping

        if (mBehavior & (kShowCloseButton | kShowLoadButton | kShowSaveButton))
        {

            if (mBehavior & kShowCloseButton)
            {
                pBtn = new ZWinSizablePushBtn();
                pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
                pBtn->SetCaption("X");
                pBtn->SetArea(rButton);
                pBtn->SetMessage(msCloseButtonMessage);
                mpPanel->ChildAdd(pBtn);

                rButton.OffsetRect(mnControlPanelButtonSide + gDefaultSpacer, 0);
            }

            if (mBehavior & kShowLoadButton)
            {
                pBtn = new ZWinSizablePushBtn();
                pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
                pBtn->SetCaption("1");  // wingdings 1 open folder
                pBtn->SetArea(rButton);
                pBtn->mStyle = wd1Style;
                pBtn->SetMessage(msLoadButtonMessage);
                mpPanel->ChildAdd(pBtn);

                rButton.OffsetRect(rButton.Width(), 0);
            }

            if (mBehavior & kShowSaveButton)
            {
                pBtn = new ZWinSizablePushBtn();
                pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
                pBtn->SetCaption("<"); // wingdings 1 save
                pBtn->mStyle = wd1Style;
                pBtn->SetArea(rButton);
                pBtn->SetMessage(msSaveButtonMessage);
                mpPanel->ChildAdd(pBtn);
            }

            rGroup.right = rButton.right;
            rGroup.bottom = rButton.bottom;
            rGroup.InflateRect(gDefaultGroupingStyle.padding, gDefaultGroupingStyle.padding);
            mpPanel->AddGrouping("File", rGroup);
            rButton.OffsetRect(rButton.Width()+gDefaultSpacer*2, 0);
        }

        // Rotation Group
        rGroup = rButton;   // start new grouping

        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption(":"); // wingdings rotate left
        pBtn->mStyle = wd3Style;
        pBtn->SetArea(rButton);
        string sMessage;
        Sprintf(sMessage, "rotate_left;target=%s", GetTargetName().c_str());
        pBtn->SetMessage(sMessage);
        mpPanel->ChildAdd(pBtn);

        rButton.OffsetRect(rButton.Width(), 0);

        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption(";"); // wingdings rotate right
        pBtn->mStyle = wd3Style;
        pBtn->SetArea(rButton);
        Sprintf(sMessage, "rotate_right;target=%s", GetTargetName().c_str());
        pBtn->SetMessage(sMessage);
        mpPanel->ChildAdd(pBtn);

        rButton.OffsetRect(rButton.Width(), 0);

        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption("1"); // wingdings flip H
        pBtn->mStyle = wd3Style;
        pBtn->SetArea(rButton);
        Sprintf(sMessage, "flipH;target=%s", GetTargetName().c_str());
        pBtn->SetMessage(sMessage);
        mpPanel->ChildAdd(pBtn);

        rButton.OffsetRect(rButton.Width(), 0);

        pBtn = new ZWinSizablePushBtn();
        pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        pBtn->SetCaption("2"); // wingdings flip V
        pBtn->mStyle = wd3Style;
        pBtn->SetArea(rButton);
        Sprintf(sMessage, "flipV;target=%s", GetTargetName().c_str());
        pBtn->SetMessage(sMessage);
        mpPanel->ChildAdd(pBtn);



        rGroup.right = rButton.right;
        rGroup.bottom = rButton.bottom;
        rGroup.InflateRect(gDefaultGroupingStyle.padding, gDefaultGroupingStyle.padding);
        mpPanel->AddGrouping("Rotate", rGroup);
        rButton.OffsetRect(gDefaultSpacer, 0);

        rButton.OffsetRect(mnControlPanelButtonSide, 0);






    }




    return ZWin::Init();
}


void ZWinImage::SetShowZoom(const ZGUI::Style& style)
{ 
    mZoomStyle = style;
}

void ZWinImage::SetCaption(const std::string& sCaption, const ZGUI::Style& style)
{
    msCaption = sCaption;
    mCaptionStyle = style;
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
    if ((mBehavior & kScrollable) && mImageArea.PtInRect(x,y))
    {
        if (SetCapture())
        {
            SetMouseDownPos(mImageArea.left-x, mImageArea.top-y);
            return true;
        }
    }

	return ZWin::OnMouseDownL(x, y);
}

bool ZWinImage::OnMouseDownR(int64_t x, int64_t y)
{
    // toggle between 100% and fullscreen
    if (mViewState != kFitToWindow)
    {
        FitImageToWindow();
    }
    else
    {
        //mfZoom = 1.0;
        SetZoom(1.0);

//        mImageArea = mpImage->GetArea();
//        mImageArea = mImageArea.CenterInRect(mAreaToDrawTo);
    }

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
    // If a zoom hotkey is set, only zoom if it's being held
    // or if no hotkey is set
    bool bEnableZoom = (mBehavior & kZoom) || (mBehavior & kHotkeyZoom && mbHotkeyActive);
    if (bEnableZoom)
    {
        // if the zoom point is outside of the image area, adjust so it's zooming around the closest pixel
        limit<int64_t>(x, mImageArea.left, mImageArea.right);
        limit<int64_t>(y, mImageArea.top, mImageArea.bottom);

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

        ZRect rImage(mpImage.get()->GetArea());

        double fZoomPointX = (double)(x - mImageArea.left);
        double fZoomPointY = (double)(y - mImageArea.top);

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
        SetZoom(fNewZoom);
        return true;
    }

    // fallthrough
    if (mpParentWin)
        mpParentWin->OnMouseWheel(x, y, nDelta);

	return true;
}

bool ZWinImage::OnKeyDown(uint32_t c)
{
    if (c == mManipulationHotkey && mbHotkeyActive == false)
    {
        mbHotkeyActive = true;
        if (mpPanel && mBehavior & kShowOnHotkey)
            mpPanel->SetVisible();
        Invalidate();
    }

    if (mpParentWin)
        return mpParentWin->OnKeyDown(c);

    return ZWin::OnKeyDown(c);
}

bool ZWinImage::OnKeyUp(uint32_t c)
{
    if (c == mManipulationHotkey)
    {
        mbHotkeyActive = false;
        if (mpPanel && mBehavior & kShowOnHotkey)
            mpPanel->SetVisible(false);
        Invalidate();
    }

    if (mpParentWin)
        return mpParentWin->OnKeyUp(c);

    return ZWin::OnKeyUp(c);
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

bool ZWinImage::LoadImage(const string& sName)
{
    mbVisible = false;
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpTransformTexture.get()->GetMutex());

    // if there's an old image, acquire the lock before freeing it

    tZBufferPtr newImage(new ZBuffer());

    if (!newImage->LoadBuffer(sName))
    {
        ZERROR("ERROR: Failed to load image:", sName);
        return false;
    }

    if (mpImage)
    {
        const std::lock_guard<std::recursive_mutex> imageLock(mpImage.get()->GetMutex());
        cout << "ZWinImage::swapping...\n";
        mpImage.swap(newImage);
    }
    else
        mpImage = newImage;

    FitImageToWindow();
    mbVisible = true;
    return true;
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

    mViewState = kFitToWindow;

    Invalidate();
}

bool ZWinImage::IsSizedToWindow()
{
    if (!mpImage)
        return false;

    return (mArea.Width() == mImageArea.Width() || mArea.Height() == mImageArea.Height());  // if either width or height matches window, then it's sized
}



void ZWinImage::SetZoom(double fZoom)
{ 
    mfZoom = fZoom;
    limit<double>(mfZoom, mfMinZoom, mfMaxZoom);

    // Update image area
    ZRect rImageArea(mpImage->GetArea());
    double fRatio = (double)rImageArea.Width() / (double)rImageArea.Height();
    int64_t nImageWidth = rImageArea.Width() * mfZoom;
    int64_t nImageHeight = (int64_t)(nImageWidth / fRatio);

    ZPoint tl(((mImageArea.left + mImageArea.right)-nImageWidth) / 2, ((mImageArea.top + mImageArea.bottom)-nImageHeight) / 2);
    mImageArea.SetRect(tl.x, tl.y, tl.x + nImageWidth, tl.y + nImageHeight);

    if (mfZoom == 1.0)
        mViewState = kZoom100;
    else if (rImageArea.Height() < mArea.Height() && mfZoom > 1.0)
        mViewState = kZoomedInToSmallImage;
    else if (rImageArea.Height() > mArea.Height() && mfZoom < 1.0)
        mViewState = kZoomedOutOfLargeImage;


    Invalidate(); 
}

double ZWinImage::GetZoom() 
{ 
    return mfZoom; 
}


void ZWinImage::SetImage(tZBufferPtr pImage)
{
    if (pImage->GetArea().Height() < mAreaToDrawTo.Height())    // if setting an image smaller than the screen, set new one unzoomed
    {
        SetZoom(1.0);
        mImageArea = pImage->GetArea();
        mImageArea = mImageArea.CenterInRect(mAreaToDrawTo);
        mpImage = pImage;
        Invalidate();
        return;
    }

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

    ZRect rDest(mpTransformTexture.get()->GetArea());

//    static int i = 0;
//    ZOUT_LOCKLESS(i++, "about to Fill texture:", mpTransformTexture.get()->GetPixels(), "pixels:", mpTransformTexture.get()->GetArea().Width() * mpTransformTexture.get()->GetArea().Height());
    mpTransformTexture.get()->Fill(mpTransformTexture.get()->GetArea(), mFillColor);
//    ZOUT_LOCKLESS("filled\n");

    ZASSERT(mpTransformTexture.get()->GetPixels() != nullptr);


//    ZOUT_LOCKLESS("mpimage");
    if (mpImage)
    {
//        ZOUT_LOCKLESS("imageSurfaceLock get");
        const std::lock_guard<std::recursive_mutex> imageSurfaceLock(mpImage.get()->GetMutex());
        if (mpImage->GetPixels())
        {
//            ZOUT_LOCKLESS("getpixels");
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
    }

    if (!msCaption.empty())
    {
        if (mBehavior & kShowCaption || (mBehavior & kShowOnHotkey && mbHotkeyActive))
        {

            ZRect rCaption(mCaptionStyle.Font()->GetOutputRect(mAreaToDrawTo, (uint8_t*)msCaption.data(), msCaption.length(), mCaptionStyle.pos));
            mCaptionStyle.Font()->DrawText(mpTransformTexture.get(), msCaption, rCaption, mCaptionStyle.look);
        }
    }

    if (mBehavior & kShowZoomCaption || (mBehavior & kShowOnHotkey && mbHotkeyActive))
    {
        string sZoom;
        Sprintf(sZoom, "%d%%", (int32_t)(mfZoom * 100.0));

        ZRect rZoomCaption(mZoomStyle.Font()->GetOutputRect(mAreaToDrawTo, (uint8_t*)sZoom.data(), sZoom.length(), mZoomStyle.pos));
        mZoomStyle.Font()->DrawText(mpTransformTexture.get(), sZoom, rZoomCaption, mZoomStyle.look);
    }

	return ZWin::Paint();
}

bool ZWinImage::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "rotate_left")
    {
        mpImage->Rotate(ZBuffer::kLeft);
        FitImageToWindow();
        return true;
    }
    else if (sType == "rotate_right")
    {
        mpImage->Rotate(ZBuffer::kRight);
        FitImageToWindow();
        return true;
    }
    else if (sType == "flipH")
    {
        mpImage->Rotate(ZBuffer::kHFlip);
        FitImageToWindow();
        return true;
    }
    else if (sType == "flipV")
    {
        mpImage->Rotate(ZBuffer::kVFlip);
        FitImageToWindow();
        return true;
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
