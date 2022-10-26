#include "SimpleColorWin.h"
#include "ZRandom.h"
#include "ZFont.h"

SimpleColorWin::SimpleColorWin()
{
    mFillColor = 0;
    mIdleSleepMS = 1000;
}
   
bool SimpleColorWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mbInvalidateParentWhenInvalid = true;
    mFillColor = RANDI64(0xff000000, 0xffffffff);

    if (mnNumChildren > 0)
    {
        for (int64_t i = mnNumChildren; i > 0; i--)
        {
            int64_t nWidth = RANDU64(20, 500);
            int64_t nHeight = RANDU64(20, 500);


            int64_t nScreenX = RANDU64(0, nWidth+mAreaToDrawTo.Width());
            int64_t nScreenY = RANDU64(0, nHeight+mAreaToDrawTo.Height());

            ZRect rSub(nScreenX, nScreenY, nScreenX + nWidth, nScreenY + nHeight);

            rSub.IntersectRect(&mAreaToDrawTo);
            if (rSub.Width() > 0 && rSub.Height() > 0)
            {
                SimpleColorWin* pWin = new SimpleColorWin();
                pWin->SetNumChildrenToSpawn(0);
                pWin->SetArea(rSub);
                ChildAdd(pWin);
            }
        }
    }

    SetFocus();

    return ZWin::Init();
}

bool SimpleColorWin::OnMouseDownR(int64_t x, int64_t y)
{
    mFillColor = RANDI64(0xff000000, 0xffffffff);
    Invalidate();

    return ZWin::OnMouseDownR(x,y);
}

bool SimpleColorWin::OnMouseDownL(int64_t x, int64_t y)
{
    if (mpParentWin)
        mpParentWin->ChildToFront(this);

    return ZWin::OnMouseDownL(x, y);
}

bool SimpleColorWin::OnKeyDown(uint32_t key)
{
#ifdef _WIN64
    if (key == VK_ESCAPE)
    {
        gMessageSystem.Post("quit_app_confirmed");
        return true;
    }
#endif

    return ZWin::OnKeyDown(key);
}


bool SimpleColorWin::Paint()
{
    if (!mbInvalid)
        return false;

    const std::lock_guard<std::mutex> surfaceLock(mpTransformTexture->GetMutex());
    mpTransformTexture->Fill(mAreaToDrawTo, mFillColor);

    string sTemp;
    Sprintf(sTemp, "id: 0x%llx", (uint64_t)this);

    gpFontSystem->GetDefaultFont(1)->DrawTextParagraph(mpTransformTexture, sTemp, mAreaToDrawTo);
    return ZWin::Paint();
}
   