#include "ZWinImage.h"
#include "ZXMLNode.h"
#include "ZAnimator.h"
#include "ZStringHelpers.h"
#include "ZRasterizer.h"
#include "ZGraphicSystem.h"

#include "Resources.h"
#include "ZGUIStyle.h"
#include "ZDebug.h"
#include "ZInput.h"

extern ZAnimator gAnimator;
using namespace std;

ZWinImage::ZWinImage()
{
	mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mpImage.reset();
	mfZoom = 1.0;
    mfPerfectFitZoom = 1.0;
    mViewState = kNoState;
    mfMinZoom = 0.01;
    mfMaxZoom = 100.0;
    mZoomHotkey = 0;
    nSubsampling = 0;
    mpTable = nullptr;
    mFillColor = 0;
    mIdleSleepMS = 10000;

    mBehavior = eBehavior::kNone;
}

ZWinImage::~ZWinImage()
{
}

bool ZWinImage::Init()
{
    if (mbInitted)
        return true;

    mCaptionMap["zoom"].style = gStyleCaption;
    mCaptionMap["zoom"].style.pos = ZGUI::LB;
//    Clear();
    return ZWin::Init();
}



bool ZWinImage::OnParentAreaChange()
{
    SetArea(mpParentWin->GetArea());

    // if previously sized to window, keep sizing to window
    if (mViewState == kFitToWindow)
        FitImageToWindow();

    return ZWin::OnParentAreaChange();
}


void ZWinImage::Clear()
{
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());
    ZOUT("ZWinImage::Clear()\n");

    mpImage.reset();
    mCaptionMap.clear();
    mCaptionMap["zoom"].style = gStyleCaption;
    mCaptionMap["zoom"].style.pos = ZGUI::CB;
    
    Invalidate();
}


bool ZWinImage::OnMouseUpL(int64_t x, int64_t y)
{
    if (AmCapturing())
    {
        if ((mBehavior & kSelectableArea) != 0 && gInput.IsKeyDown(VK_SHIFT))
        {
            ZRect rSelection = GetSelection();
            ToImageCoords(rSelection);
            gMessageSystem.Post("image_selection", mpParentWin, "r", RectToString(rSelection));
        }

        Invalidate();
        ReleaseCapture();
    }
    if (!msMouseUpLMessage.empty() && mImageArea.PtInRect(x, y))
        gMessageSystem.Post(msMouseUpLMessage);
    return ZWin::OnMouseUpL(x, y);
}

bool ZWinImage::OnMouseDownL(int64_t x, int64_t y)
{
    if (mpImage)
    {
        if ((mBehavior & kNotifyOnClick) != 0 && mpParentWin)
        {
            // convert coordinates to parent's
            LocalToAbs(x, y);
            mpParentWin->AbsToLocal(x, y);

            mpParentWin->OnMouseDownL(x, y);
        }

        if ((mBehavior & kSelectableArea) != 0 && gInput.IsKeyDown(VK_SHIFT))
        {
            if (SetCapture())
            {
                if (x < 0)
                    x = 0;
                if (y < 0)
                    y = 0;

                SetMouseDownPos(x,y);
                return true;
            }
        }

        if ((mBehavior & kLaunchGeolocation) != 0)
        {
            easyexif::EXIFInfo& exif = mpImage->GetEXIF();
            bool bHasGeoLocation = exif.GeoLocation.Latitude != 0.0 || exif.GeoLocation.Longitude != 0.0;

#ifdef _WIN32
            if (((mBehavior & kShowCaption) != 0) && bHasGeoLocation && mpTable && mpTable->mrAreaToDrawTo.PtInRect(x, y))
            {
                // open geolocation https://maps.google.com/?q=<lat>,<lng>
                string sURL;
                Sprintf(sURL, "https://maps.google.com/?t=k&q=%f,%f", exif.GeoLocation.Latitude, exif.GeoLocation.Longitude);
                ShellExecute(0, "open", sURL.c_str(), 0, 0, SW_SHOWNORMAL);

                return true;
            }
#endif
        }
        if ((mBehavior & kScrollable) && mImageArea.PtInRect(x, y))
        {
            if (SetCapture())
            {
                SetMouseDownPos(mImageArea.left - x, mImageArea.top - y);
                return true;
            }
        }
    }

	return ZWin::OnMouseDownL(x, y);
}

bool ZWinImage::OnMouseDownR(int64_t x, int64_t y)
{
    if (mpImage)
    {
        // toggle between 100% and fullscreen
        if (mViewState != kFitToWindow)
        {
            FitImageToWindow();
        }
        else
        {
            double fZoomPointX = (double)(x - mImageArea.left);
            double fZoomPointY = (double)(y - mImageArea.top);

            double fZoomPointU = fZoomPointX / (double)mImageArea.Width();
            double fZoomPointV = fZoomPointY / (double)mImageArea.Height();

            ZRect rImage(mpImage.get()->GetArea());



            int64_t nNewLeft = (int64_t)(x - (rImage.Width() * fZoomPointU));
            int64_t nNewRight = nNewLeft + (int64_t)rImage.Width();

            int64_t nNewTop = (int64_t)(y - (rImage.Height() * fZoomPointV));
            int64_t nNewBottom = nNewTop + (int64_t)rImage.Height();

            ZRect rNewArea(nNewLeft, nNewTop, nNewRight, nNewBottom);

            mImageArea.SetRect(rNewArea);
            SetZoom(1.0);
            Invalidate();
        }
    }

    return ZWin::OnMouseDownR(x, y);
}


bool ZWinImage::OnMouseMove(int64_t x, int64_t y)
{
    // x,y in window space

    if (AmCapturing())
    {
        if ((mBehavior & kSelectableArea) != 0 && gInput.IsKeyDown(VK_SHIFT))
        {
            Invalidate();   // paint function to show highlight
            return true;
        }

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
    bool bEnableZoom = (mBehavior & kZoom) || (mBehavior & kHotkeyZoom && gInput.IsKeyDown(mZoomHotkey));
    if (bEnableZoom && mpImage)
    {
        // if the zoom point is outside of the image area, adjust so it's zooming around the closest pixel
        limit<int64_t>(x, mImageArea.left, mImageArea.right);
        limit<int64_t>(y, mImageArea.top, mImageArea.bottom);

        double fNewZoom = mfZoom;
        if (nDelta < 0)
        {
            if (mfZoom >= mfMaxZoom)
                return true;

            fNewZoom *= 1.2f;
        }
        else
        {
            if (mfZoom <= mfMinZoom)
                return true;

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
    if (mpParentWin)
    {
        if (c == mZoomHotkey)
            mpParentWin->InvalidateChildren();
        return mpParentWin->OnKeyDown(c);
    }

    return ZWin::OnKeyDown(c);
}

bool ZWinImage::OnKeyUp(uint32_t c)
{
    if (AmCapturing() && (mBehavior & kSelectableArea) != 0 && c == VK_SHIFT)
    {
        ReleaseCapture();
        mpParentWin->InvalidateChildren();
    }


    if (c == mZoomHotkey)
        mpParentWin->InvalidateChildren();

    return mpParentWin->OnKeyUp(c);
}


void ZWinImage::ScrollTo(int64_t nX, int64_t nY)
{
    const int32_t kSnapDistance = 10;
    if (abs(nX - mAreaLocal.left) < kSnapDistance)
    {
        nX = mAreaLocal.left;    // snap left
    }
    else if (abs(nX + mImageArea.Width() - mAreaLocal.right) < kSnapDistance)
    {
        nX = mAreaLocal.right - mImageArea.Width(); // snap right
    }

    if (abs(nY - mAreaLocal.top) < kSnapDistance)
    {
        nY = mAreaLocal.top;    // snap top
    }
    else if (abs(nY + mImageArea.Height() - mAreaLocal.bottom) < kSnapDistance)
    {
        nY = mAreaLocal.bottom - mImageArea.Height(); // snap right
    }

    mImageArea.MoveRect(nX, nY);
    Invalidate();
}

bool ZWinImage::LoadImage(const string& sName)
{
    mbVisible = false;
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

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

    if (mViewState == kFitToWindow)
        FitImageToWindow();
    mbVisible = true;
}


void ZWinImage::FitImageToWindow()
{
    mViewState = kFitToWindow;

    if (!mpImage)
        return;

    mImageArea = ZGUI::ScaledFit(mpImage->GetArea(), mAreaLocal);
    mfZoom = (double)mImageArea.Width() / (double)mpImage->GetArea().Width();
    mfPerfectFitZoom = mfZoom;

    Invalidate();
}

bool ZWinImage::IsSizedToWindow()
{
    if (!mpImage)
        return false;

    return (mAreaInParent.Width() == mImageArea.Width() || mAreaInParent.Height() == mImageArea.Height());  // if either width or height matches window, then it's sized
}



void ZWinImage::SetZoom(double fZoom)
{ 
    mfZoom = fZoom;
    limit<double>(mfZoom, mfMinZoom, mfMaxZoom);

    // Update image area
    if (mpImage)
    {
        ZRect rImageArea(mpImage->GetArea());
        double fRatio = (double)rImageArea.Width() / (double)rImageArea.Height();
        int64_t nImageWidth = (int64_t)(rImageArea.Width() * mfZoom);
        int64_t nImageHeight = (int64_t)(nImageWidth / fRatio);

        ZPoint tl(((mImageArea.left + mImageArea.right) - nImageWidth) / 2, ((mImageArea.top + mImageArea.bottom) - nImageHeight) / 2);
        mImageArea.SetRect(tl.x, tl.y, tl.x + nImageWidth, tl.y + nImageHeight);

        if (mfZoom == 1.0)
            mViewState = kZoom100;
        else if (rImageArea.Height() < mAreaInParent.Height() && mfZoom > 1.0)
            mViewState = kZoomedInToSmallImage;
        else if (rImageArea.Height() > mAreaInParent.Height() && mfZoom < 1.0)
            mViewState = kZoomedOutOfLargeImage;
    }

    Invalidate(); 
}

double ZWinImage::GetZoom() 
{ 
    return mfZoom; 
}


void ZWinImage::SetImage(tZBufferPtr pImage)
{
    ZRect rOldImage;
    if (mpImage)
        rOldImage = mpImage->GetArea();

    mpImage = pImage;
    const std::lock_guard<std::recursive_mutex> imageSurfaceLock(mpImage.get()->GetMutex());
    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

    if (mViewState == kNoState)
    {
        if (mpImage->GetArea().Width() > mAreaLocal.Width() || mpImage->GetArea().Height() > mAreaLocal.Height())
            FitImageToWindow();
        else
        {
            SetZoom(1.0);
            mImageArea = pImage->GetArea();
            mImageArea = mImageArea.CenterInRect(mAreaLocal);
        }
    }
    else if (mViewState == kFitToWindow)
    {
        FitImageToWindow();
    }
    else
    {
        SetZoom(mfZoom);
    }



    Invalidate();
}

bool ZWinImage::Paint()
{
    if (!mpSurface)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

    if (!mbVisible)
        return false;

    if (!mbInvalid)
        return false;


    ZASSERT(mpSurface.get()->GetPixels() != nullptr);

    ZRect rDest(mpSurface.get()->GetArea());

    mpSurface.get()->Fill(mFillColor);

    ZASSERT(mpSurface.get()->GetPixels() != nullptr);

    tZBufferPtr pRenderImage = mpImage;
    ZRect rRenderArea = mImageArea;

//    ZOUT_LOCKLESS("mpimage");
    if (pRenderImage)
    {
//        ZOUT_LOCKLESS("imageSurfaceLock get");
        if (pRenderImage->GetPixels())
        {
//            ZOUT_LOCKLESS("getpixels");
            ZRect rSource(pRenderImage->GetArea());

//            ZOUT("Rendering image:", pRenderImage->GetEXIF().DateTime, "\n");

            if (mfZoom == 1.0f && rDest == rSource)  // simple blt?
            {
                mpSurface.get()->Blt(pRenderImage.get(), rSource, rDest);
            }
            else
            {
                tUVVertexArray verts;
                gRasterizer.RectToVerts(rRenderArea, verts);
                ZASSERT(mpSurface.get()->GetPixels() != nullptr);

                if (nSubsampling == 0 || AmCapturing() || gInput.IsKeyDown(mZoomHotkey) || mfZoom == 1.00)
                    gRasterizer.RasterizeWithAlpha(mpSurface.get(), pRenderImage.get(), verts, &mAreaLocal);
                else
                    gRasterizer.MultiSampleRasterizeWithAlpha(mpSurface.get(), pRenderImage.get(), verts, &mAreaLocal, nSubsampling);
            }
        }
    }

    if (AmCapturing() && (mBehavior & kSelectableArea) != 0 && gInput.IsKeyDown(VK_SHIFT))
    {
        mpSurface->FillAlpha(0x88555588, &GetSelection());
    }


    Sprintf(mCaptionMap["zoom"].sText, "%d%%", (int32_t)(mfZoom * 100.0));
    mCaptionMap["zoom"].visible = gInput.IsKeyDown(mZoomHotkey);

    if ((mBehavior & kShowCaption) != 0)
    {
        ZGUI::TextBox::Paint(mpSurface.get(), mCaptionMap);

        if (pRenderImage && mpTable)
            mpTable->Paint(mpSurface.get());
    }

	return ZWin::Paint();
}

ZRect ZWinImage::GetSelection()
{
    int64_t nW = gInput.lastMouseMove.x- mMouseDownOffset.x;
    int64_t nH = gInput.lastMouseMove.y- mMouseDownOffset.y;

    if (abs(nW) > abs(nH))
        nW = (int64_t)((double)nW / (double)abs(nW) * (double)abs(nH));
    else
        nH = (int64_t)((double)nH / (double)abs(nH) * (double)abs(nW));

    ZRect rSelection(mMouseDownOffset.x, mMouseDownOffset.y, mMouseDownOffset.x+nW, mMouseDownOffset.y+nH);
    rSelection.NormalizeRect();

    if (rSelection.Width() > rSelection.Height())
        rSelection.right = rSelection.left + rSelection.Height();
    else
        rSelection.bottom = rSelection.top + rSelection.Width();

    limit<int64_t>(rSelection.left, mImageArea.left, mImageArea.right);
    limit<int64_t>(rSelection.right, mImageArea.left, mImageArea.right);
    limit<int64_t>(rSelection.top, mImageArea.top, mImageArea.bottom);
    limit<int64_t>(rSelection.bottom, mImageArea.top, mImageArea.bottom);

    return rSelection;
}

void ZWinImage::ToImageCoords(ZRect& r)
{
    r.OffsetRect(-mImageArea.left, -mImageArea.top);
    int64_t nW = r.Width();
    int64_t nH = r.Height();

    r.left = (int64_t)(r.left /mfZoom);
    r.top = (int64_t)(r.top/mfZoom);

    r.right = r.left + (int64_t)(nW / mfZoom);
    r.bottom = r.top + (int64_t)(nH / mfZoom);
}


bool ZWinImage::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();


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
