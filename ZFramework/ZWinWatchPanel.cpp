#include "ZWinWatchPanel.h"
#include "ZFont.h"
#include "ZStringHelpers.h"
#include "Resources.h"

using namespace std;

ZWinWatchPanel::ZWinWatchPanel()
{
    mbAcceptsCursorMessages = true;
    mIdleSleepMS = 25000;
}

bool ZWinWatchPanel::Init()
{
    mrNextControl.SetRect(gnControlPanelEdge, gnControlPanelEdge, mAreaInParent.Width() - gnControlPanelEdge, gnControlPanelEdge + gnControlPanelButtonHeight);

    return ZWin::Init();
}


bool ZWinWatchPanel::AddItem(WatchType type, const string& sCaption, void* pWatchAddress, const ZGUI::ZTextLook& captionLook, const ZGUI::ZTextLook& watchLook)
{
    WatchStruct newWatch;
    newWatch.mCaption = sCaption;
    newWatch.mType = type;
    newWatch.pMem = pWatchAddress;
    newWatch.captionLook = captionLook;
    newWatch.mLook = watchLook;
    newWatch.mAreaInParent = mrNextControl;
    newWatch.mCaption = sCaption;

    mrNextControl.OffsetRect(0, mrNextControl.Height());

    mWatchList.emplace_back(newWatch);

    if (type == WatchType::kString)
    {
        mWatchedStrings.push_back(*(string*)pWatchAddress);
    }
    else if (type == WatchType::kInt64)
    {
        mWatchedInt64s.push_back(*(int64_t*)pWatchAddress);
    }
    else if (type == WatchType::kDouble)
    {
        mWatchedDoubles.push_back(*(double*)pWatchAddress);
    }
    else if (type == WatchType::kBool)
    {
        mWatchedBools.push_back(*(bool*) pWatchAddress);
    }


    return true;
}

bool ZWinWatchPanel::Process()
{
    bool bChanged = false;

    int32_t nWatchedStringIndex = 0;
    int32_t nWatchedInt64Index = 0;
    int32_t nWatchedDoubleIndex = 0;
    int32_t nWatchedBoolIndex = 0;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());  // these are modified within paint

    for (auto ws : mWatchList)
    {
        if (ws.mType == WatchType::kString)
        {
            if (mWatchedStrings.size() <= nWatchedStringIndex)
            {
                mbInvalid = true;
                break;
            }

            string sValue = mWatchedStrings[nWatchedStringIndex++];

            if (*(string*)ws.pMem != sValue)
            {
                mbInvalid = true;
                break;
            }
        }
        else if (ws.mType == WatchType::kInt64)
        {
            if (mWatchedInt64s.size() <= nWatchedInt64Index)
            {
                mbInvalid = true;
                break;
            }

            int64_t nValue = mWatchedInt64s[nWatchedInt64Index++];
            if (*(int64_t*)ws.pMem != nValue)
            {
                mbInvalid = true;
                break;
            }
        }
        else if (ws.mType == WatchType::kDouble)
        {
            if (mWatchedDoubles.size() <= nWatchedDoubleIndex)
            {
                mbInvalid = true;
                break;
            }

            double fValue = mWatchedDoubles[nWatchedDoubleIndex++];
            if (*(double*)ws.pMem != fValue)
            {
                mbInvalid = true;
                break;
            }
        }
        else if (ws.mType == WatchType::kBool)
        {
            if (mWatchedBools.size() <= nWatchedBoolIndex)
            {
                mbInvalid = true;
                break;
            }

            bool bValue = mWatchedBools[nWatchedBoolIndex++];
            if (*(bool*)ws.pMem != bValue)
            {
                mbInvalid = true;
                break;
            }
        }
    }

    return ZWin::Process();
}

bool ZWinWatchPanel::Paint()
{
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface.get()->Fill(gDefaultDialogFill);

    mWatchedStrings.clear();
    mWatchedInt64s.clear();
    mWatchedDoubles.clear();
    mWatchedBools.clear();

    for (auto ws : mWatchList)
    {
        tZFontPtr pFont = gpFontSystem->GetDefaultFont();

        ZGUI::Style captionStyle(pFont->GetFontParams(), ws.captionLook, ZGUI::LB);
        ZGUI::Style textStyle(pFont->GetFontParams(), ws.textLook, ZGUI::RB);

        pFont->DrawTextParagraph(mpSurface.get(), ws.mCaption, ws.mAreaInParent, &captionStyle);

        if (ws.mType == WatchType::kString)
        {
            string sValue = *((string*)ws.pMem);
            mWatchedStrings.push_back(sValue);
            pFont->DrawTextParagraph(mpSurface.get(), sValue, ws.mAreaInParent, &textStyle);
        }
        else if (ws.mType == WatchType::kInt64)
        {
            int64_t nValue = *((int64_t*)ws.pMem);
            mWatchedInt64s.push_back(nValue);

            string sValue;
            Sprintf(sValue, "%lld", nValue);
            pFont->DrawTextParagraph(mpSurface.get(), sValue, ws.mAreaInParent, &textStyle);
        }
        else if (ws.mType == WatchType::kDouble)
        {
            double fValue = *((double*)ws.pMem);
            mWatchedDoubles.push_back(fValue);
            string sValue;
            Sprintf(sValue, "%LF", fValue);
            pFont->DrawTextParagraph(mpSurface.get(), sValue, ws.mAreaInParent, &textStyle);
        }
        else if (ws.mType == WatchType::kBool)
        {
            bool bValue = *((bool*)ws.pMem);
            mWatchedBools.push_back(bValue);
            string sValue;
            if (bValue == true)
                sValue = "1";
            else 
                sValue = "0";
            pFont->DrawTextParagraph(mpSurface.get(), sValue, ws.mAreaInParent, &textStyle);
        }
    }

    return ZWin::Paint();
}
