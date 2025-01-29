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
    mbConditionalPaintCaptions = false;
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

    if (msWinName.empty())
        msWinName = "ZWinImage_" + gMessageSystem.GenerateUniqueTargetName();
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
    {
        FitImageToWindow();
    }
    else if (mViewState == kZoom100)
    {
        CenterImage();
    }

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
            mrSelection = GetSelection();
            ZRect rImageCoords(mrSelection);
            ToImageCoords(rImageCoords);
            gMessageSystem.Post(ZMessage("image_selection", mpParentWin, "r", RectToString(rImageCoords)));
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
    ClearSelection();
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

                mMouseDownOffset.Set(x, y);
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
                mMouseDownOffset.Set(mImageArea.left - x, mImageArea.top - y);
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

            SetZoom(1.0);

            if (rImage.Width() > mAreaLocal.Width() || rImage.Height() > mAreaLocal.Height())
            {
                int64_t nNewLeft = (int64_t)(x - (rImage.Width() * fZoomPointU));
                int64_t nNewRight = nNewLeft + (int64_t)rImage.Width();

                int64_t nNewTop = (int64_t)(y - (rImage.Height() * fZoomPointV));
                int64_t nNewBottom = nNewTop + (int64_t)rImage.Height();

                ZRect rNewArea(nNewLeft, nNewTop, nNewRight, nNewBottom);

                mImageArea.SetRect(rNewArea);
            }
            else
            {
                CenterImage();
            }

        }
    }

    return ZWin::OnMouseDownR(x, y);
}


bool ZWinImage::OnMouseMove(int64_t x, int64_t y)
{
    if ((mBehavior & kShowCaptionOnMouseOver) != 0)
    {
        if (gInput.lastMouseMove.x < 20 || gInput.lastMouseMove.x > mAreaLocal.right - 20 || gInput.lastMouseMove.y < 20 || gInput.lastMouseMove.y > mAreaLocal.bottom - 20)
        {
            if (mbConditionalPaintCaptions == false)
                Invalidate();
            mbConditionalPaintCaptions = true;
        }
        else
        {
            if (mbConditionalPaintCaptions == true)
                Invalidate();
            mbConditionalPaintCaptions = false;
        }
    }



    if (AmCapturing())
    {
        if ((mBehavior & kSelectableArea) != 0 && gInput.IsKeyDown(VK_SHIFT))
        {
            mrSelection = GetSelection();
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
    UpdateZoomIcon();
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
    if (mpSurface)
    {
        mpSurface->mMutex.lock();
        mpSurface->mRenderState = ZBuffer::kBusy_SkipRender;
        mpSurface->mMutex.unlock();
    }
    ZWin::SetArea(newArea);

    if (mViewState == kFitToWindow)
        FitImageToWindow();
}


void ZWinImage::FitImageToWindow()
{
    mViewState = kFitToWindow;

    if (!mpImage)
        return;

    mImageArea = ZGUI::ScaledFit(mpImage->GetArea(), mAreaLocal);
    mfZoom = (double)mImageArea.Width() / (double)mpImage->GetArea().Width();
    mfPerfectFitZoom = mfZoom;
    UpdateZoomIcon();

    Invalidate();
}

void ZWinImage::CenterImage()
{
    mImageArea.SetRect(ZGUI::Arrange(mImageArea, mAreaLocal, ZGUI::C));
    UpdateZoomIcon();
    Invalidate();
}



bool ZWinImage::IsSizedToWindow()
{
    if (!mpImage)
        return false;

    return (mAreaInParent.Width() == mImageArea.Width() || mAreaInParent.Height() == mImageArea.Height());  // if either width or height matches window, then it's sized
}


void ZWinImage::UpdateZoomIcon()
{
    // zoom icon
    ZRect rImageArea = mAreaLocal;
    ZRect rZoomedImageArea = mImageArea;

    ZGUI::ImageBox& imgBox = mIconMap["zoomicon"];

    if (rImageArea.Width() == 0 || rImageArea.Height() == 0 || rZoomedImageArea.Width() == 0 || rZoomedImageArea.Height() == 0)
    {
        imgBox.visible = false;
        return;
    }

    rZoomedImageArea.OffsetRect(mAreaInParent.TL());  // adjust to local image coordinates

    ZRect rBounds(rZoomedImageArea);
    rBounds.UnionRect(rImageArea);

    double fBoundsRatio = (double)rBounds.Height() / (double)rBounds.Width();

    int64_t iconW = gM * 4;
    int64_t iconH = iconW * fBoundsRatio;

    double fIconScale = (double)iconH / (double)rBounds.Height();

    ZRect rScreenIcon(rImageArea.left * fIconScale, rImageArea.top * fIconScale, rImageArea.right * fIconScale, rImageArea.bottom * fIconScale);
    ZRect rImageIcon(rZoomedImageArea.left * fIconScale, rZoomedImageArea.top * fIconScale, rZoomedImageArea.right * fIconScale, rZoomedImageArea.bottom * fIconScale);

    rScreenIcon.OffsetRect(-rBounds.left * fIconScale, -rBounds.top * fIconScale);
    rImageIcon.OffsetRect(-rBounds.left * fIconScale, -rBounds.top * fIconScale);



    imgBox.area = ZGUI::Arrange(ZRect(0, 0, iconW, iconH), rImageArea, ZGUI::LB, gSpacer, gSpacer);
    ZRect rDest(imgBox.RenderRect());

    if (!imgBox.mRendered || imgBox.mRendered->GetArea().Width() != rDest.Width() || imgBox.mRendered->GetArea().Height() != rDest.Height())
    {
        imgBox.mRendered.reset(new ZBuffer());
        imgBox.mRendered->Init(rDest.Width(), rDest.Height());
    }

    imgBox.mRendered->Fill(0);
    ZRect r(imgBox.mRendered->GetArea());

    imgBox.mRendered->DrawRectAlpha(0x99ffffff, rScreenIcon, ZBuffer::kAlphaSource);

    imgBox.mRendered->DrawRectAlpha(0xff0000ff, rImageIcon, ZBuffer::kAlphaSource);


    imgBox.visible = true;

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

        UpdateZoomIcon();
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
            UpdateZoomIcon();
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
    if (!PrePaintCheck())
        return false;

    //ZDEBUG_OUT_LOCKLESS("ZWinImage::Paint() - in...");

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());


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

    if (mrSelection.Area())
    {
        mpSurface->FillAlpha(0x88555588, &mrSelection);
        mpSurface->DrawRectAlpha(0xff555588, &mrSelection);
    }


    Sprintf(mCaptionMap["zoom"].sText, "%d%%", (int32_t)(mfZoom * 100.0));
    mCaptionMap["zoom"].visible = gInput.IsKeyDown(mZoomHotkey);

    bool bPaintCaptions = mbConditionalPaintCaptions || (mBehavior & kShowCaption) != 0;

    if (bPaintCaptions)
    {
        ZGUI::TextBox::Paint(mpSurface.get(), mCaptionMap);

        if (pRenderImage && mpTable)
            mpTable->Paint(mpSurface.get());
    }
    ZGUI::ImageBox::Paint(mpSurface.get(), mIconMap);

//    ZDEBUG_OUT_LOCKLESS("out...\n");

	return ZWin::Paint();
}

void ZWinImage::ClearSelection()
{
    mrSelection.SetRect(-1, -1, -1, -1);
    Invalidate();
}

ZRect ZWinImage::GetSelection()
{
    ZPoint lastMoveWinCoord(gInput.lastMouseMove);
    lastMoveWinCoord.Offset(-mAreaAbsolute.left, -mAreaAbsolute.top);
    int64_t nW = lastMoveWinCoord.x - mMouseDownOffset.x;
    int64_t nH = lastMoveWinCoord.y - mMouseDownOffset.y;

    ZRect rSelection;

    if (gInput.IsKeyDown(VK_CONTROL))
    {
        if (abs(nW) > abs(nH))
            nW = (int64_t)((double)nW / (double)abs(nW) * (double)abs(nH));
        else
            nH = (int64_t)((double)nH / (double)abs(nH) * (double)abs(nW));

        rSelection.SetRect(mMouseDownOffset.x, mMouseDownOffset.y, mMouseDownOffset.x+nW, mMouseDownOffset.y+nH);
        rSelection.NormalizeRect();

        if (rSelection.Width() > rSelection.Height())
            rSelection.right = rSelection.left + rSelection.Height();
        else
            rSelection.bottom = rSelection.top + rSelection.Width();
    }
    else
    {
        rSelection.SetRect(mMouseDownOffset.x, mMouseDownOffset.y, mMouseDownOffset.x + nW, mMouseDownOffset.y + nH);
    }

    rSelection.IntersectRect(mImageArea);   // clip

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
    if (sType == "clear_selection")
    {
        ClearSelection();
        return true;
    }

    return ZWin::HandleMessage(message);
}