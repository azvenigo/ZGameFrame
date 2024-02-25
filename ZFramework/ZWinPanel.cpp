#include "ZWinPanel.h"
#include "ZWinSlider.h"
#include "ZWinImage.h"
#include "ZWinText.H"
#include "ZWinBtn.h"
#include "ZWinFormattedText.h"
#include "ZStringHelpers.h"
#include "ZMessageSystem.h"
#include "ZXMLNode.h"
#include "ZGraphicSystem.h"
#include "helpers/StringHelpers.h"
#include "Resources.h"

using namespace std;




// defining a panel layout
// will consist of rows of controls
// each row has sets of controls with attributes
// panel will have behavior flags

// <panel   attribs>
//     <row> <control attribs/>  </row>
// </panel>




ZWinPanel::ZWinPanel() : mBehavior(kNone)
{
    mRows = 0;
    mIdleSleepMS = 250;

    if (msWinName.empty())
        msWinName = "ZWinPanel_" + gMessageSystem.GenerateUniqueTargetName();

    mStyle = gDefaultPanelStyle;
    mGroupingStyle = gDefaultGroupingStyle;
    mbAcceptsCursorMessages = true;
    mbMouseWasOver = false;
    mDrawBorder = false;
    mSpacers = 0;        
}

ZWinPanel::~ZWinPanel()
{
}


bool ZWinPanel::Init()
{
    if (mbInitted)
        return true;

    ResolveLayoutTokens();
    if (!ParseLayout())
        return false;

    UpdateUI();
	return ZWin::Init();
}

bool ZWinPanel::Shutdown()
{
	return ZWin::Shutdown();
};

bool ZWinPanel::Paint()
{
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface->GetMutex());
   
    if (ARGB_A(mStyle.bgCol) > 0x0f)
        mpSurface->Fill(mStyle.bgCol);
    if (mDrawBorder)
    {
        ZRect rBounds(mAreaLocal);
     //   rBounds.DeflateRect(gSpacer, gSpacer);
        mpSurface->DrawRectAlpha(0x88000000, rBounds);
        rBounds.OffsetRect(1, 1);
        mpSurface->DrawRectAlpha(0x88ffffff, rBounds);
    }


    tGroupNameToWinList groups = GetChildGroups();
    for (auto& group : groups)
    {
        // draw embossed rect
        ZRect rBounds(GetBounds(group.second));
        rBounds.InflateRect(gSpacer, gSpacer);
        rBounds.right--;
        rBounds.bottom--;

        mpSurface->DrawRectAlpha(0x88000000, rBounds);
        rBounds.OffsetRect(1, 1);
        mpSurface->DrawRectAlpha(0x88ffffff, rBounds);

        mGroupingStyle.Font()->DrawTextParagraph(mpSurface.get(), group.first, rBounds, &mGroupingStyle);



        // if caption set, draw that

    }

	return ZWin::Paint();
}


bool ZWinPanel::Process()
{
    if (IsSet(kHideOnMouseExit))
    {
        if (gInput.captureWin == nullptr)   // only process this if no window has capture
        {
            if (mbVisible)
            {
                ZRect rOverArea(mAreaAbsolute);
                if (rOverArea.PtInRect(gInput.lastMouseMove))
                {
                    mbMouseWasOver = true;
                }
                else
                {
                    if (mbMouseWasOver)
                    {
                        SetVisible(false);
                        if (mpParentWin)
                            mpParentWin->InvalidateChildren();
                        return true;
                    }
                }
            }
        }
    }

    return ZWin::Process();
}

bool ZWinPanel::ParseLayout()
{
	string sElement;

	ZXMLNode tree;
    tree.Init(mPanelLayout);

    ZXMLNode* pPanel = tree.GetChild("panel");
	if (!pPanel)
		return false;

	// Panel Attributes
    //      Behavior
    if (SH::ToBool(pPanel->GetAttribute("hide_on_mouse_exit")))
        mBehavior |= kHideOnMouseExit;
    if (SH::ToBool(pPanel->GetAttribute("hide_on_button")))
        mBehavior |= kHideOnButton;

    if (pPanel->HasAttribute("spacers"))
        mSpacers = SH::ToInt(pPanel->GetAttribute("spacers"));
    mDrawBorder = SH::ToBool(pPanel->GetAttribute("border"));


    //      Style?
    


	// Rows
	tXMLNodeList rowList;
    pPanel->GetChildren("row", rowList);

	for (auto pRow : rowList)
	{
        if (!ParseRow(pRow))
            return false;
        mRows++;
	}

	return true;
}

bool ZWinPanel::Registered(const std::string& _name)
{
    for (auto& r : mRegistered)
    {
        if (r.first == _name)
            return true;
    }

    return false;
}

tWinList ZWinPanel::GetRowWins(int32_t row)
{
    tWinList wins;
    if (row >= 0 && row < mRows)
    {
        for (auto& r : mRegistered)
        {
            if (r.second.row == row)
                wins.push_back(GetChildWindowByWinName(r.first));
        }
    }

    return wins;
}


bool ZWinPanel::ParseRow(ZXMLNode* pRow)
{
	ZASSERT(pRow);
    tXMLNodeList controls;
    pRow->GetChildren(controls);

    // for each 
    for (auto control : controls)
    {
        string type = control->GetName();
        string id = control->GetAttribute("id");

        string msgPrefix;
        string msgSuffix;
        if (IsSet(kHideOnButton))
        {
            msgPrefix = "{";
            msgSuffix = "}{set_visible;visible=0;target=" + GetTargetName() + "}";
        }

        if (id.empty() || Registered(id))
        {
            ZERROR("ID:", id, " invalid. Must have unique IDs for each registered window.");
            return false;
        }

        ZWin* pWin = nullptr;

        if (type == "button")
        {
            // add a button
            pWin = new ZWinSizablePushBtn();
            ZWinSizablePushBtn* pBtn = (ZWinSizablePushBtn*)pWin;
            pBtn->mCaption.sText = control->GetAttribute("caption");
            pBtn->mCaption.autoSizeFont = true;
            pBtn->msButtonMessage = msgPrefix+control->GetAttribute("msg") + msgSuffix;
        }
        else if (type == "svgbutton")
        {
            // add svg button
            pWin = new ZWinSizablePushBtn();
            ZWinSizablePushBtn* pBtn = (ZWinSizablePushBtn*)pWin;
            pBtn->mSVGImage.Load(control->GetAttribute("svgpath"));
            pBtn->msButtonMessage = msgPrefix+control->GetAttribute("msg") + msgSuffix;
        }
        else if (type == "toggle")
        {
            // add toggle
            pWin = new ZWinCheck();
            ZWinCheck* pCheck = (ZWinCheck*)pWin;
            pCheck->msButtonMessage = msgPrefix+control->GetAttribute("checkmsg") + msgSuffix;
            pCheck->msUncheckMessage = msgPrefix+control->GetAttribute("uncheckmsg") + msgSuffix;
        }
        else if (type == "label")
        {
            // add label
            pWin = new ZWinLabel();
            ZWinLabel* pLabel = (ZWinLabel*)pWin;
            pLabel->msText = control->GetAttribute("caption");
            pLabel->mBehavior |= ZWinLabel::eBehavior::AutoSizeText;
        }
        else
        {
            ZASSERT(false);
            ZERROR("Unsupported type:", type);
            return false;
        }

        // Common attributes
        pWin->msWinGroup = control->GetAttribute("group");
        pWin->msTooltip = control->GetAttribute("tooltip");
        pWin->msWinName = id;
        ChildAdd(pWin);

        mRegistered[id].row = mRows;
    }

    return true;
}

bool ZWinPanel::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "closepanel")
    {
        if (mpParentWin)
            mpParentWin->SetFocus();
        gMessageSystem.Post("kill_child", "name", msWinName);
        return true;
    }

	return ZWin::HandleMessage(message);
}

void ZWinPanel::UpdateUI()
{
    if (mRows > 0 && mAreaLocal.Area())
    {
        ZRect controlArea = mAreaLocal;
        controlArea.DeflateRect(mStyle.paddingH, mStyle.paddingV);

        int32_t spaceBetweenRows = mSpacers * gSpacer;
        int32_t totalSpaceBetweenRows = (mRows - 1) * spaceBetweenRows;

        int32_t rowh = (controlArea.Height()-totalSpaceBetweenRows)/mRows;

        for (int32_t row = 0; row < mRows; row++)
        {
            tWinList rowWins = GetRowWins(row);
            if (!rowWins.empty())
            {
                int32_t w = controlArea.Width() / rowWins.size();

                int col = 0;
                for (auto pWin : rowWins)
                {
                    ZPoint tl(controlArea.left + col*w, controlArea.top + row*(rowh + spaceBetweenRows));
                    ZRect r(tl.x, tl.y, tl.x+w, tl.y+rowh);
                    pWin->SetArea(r);
                    col++;
                }
            }
        }
    }
}

void ZWinPanel::SetRelativeArea(const ZRect& area, const ZRect& ref, ZGUI::ePosition pos)
{
    assert(IsSet(kRelativeScale));
    mRelativeArea = ZGUI::RelativeArea(area, ref, pos);
    SetArea(area);
    UpdateUI();
}


bool ZWinPanel::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    if (IsSet(kRelativeScale))
    {
        ZRect rNewParent(mpParentWin->GetArea());
        SetArea(mRelativeArea.Area(rNewParent));
    }


    ZWin::OnParentAreaChange();
    UpdateUI();

    return true;
}

void ZWinPanel::SetVisible(bool bVisible)
{
    if (bVisible)
    {
        mIdleSleepMS = 250;
    }
    else
    {
        mIdleSleepMS = 30000;
        mbMouseWasOver = false;
    }

    UpdateUI();
    return ZWin::SetVisible(bVisible);
}

bool ZWinPanel::ResolveLayoutTokens()
{
    for (size_t i = 0; i < mPanelLayout.size(); i++)
    {
        if (mPanelLayout[i] == '%')
        {
            for (size_t j = i+1; j < mPanelLayout.size(); j++)
            {
                if (mPanelLayout[j] == '%')
                {
                    string sToken = mPanelLayout.substr(i+1, j - i - 1);    // pull token name
                    if (!sToken.empty())
                    {
                        string sValue = ResolveToken(sToken);
                        if (!sValue.empty())
                        {
                            string sNew = mPanelLayout.substr(0, i);
                            sNew += sValue;
                            sNew += mPanelLayout.substr(j + 1);
                            mPanelLayout = sNew;
                            i += sValue.length() - sToken.length() + 2;         // adjust the iterator to skip the replaced token. +2 to skip the leading and end '%'
                            break;
                        }
                    }
                }
            }
        }
    }

    return true;
}


string ZWinPanel::ResolveToken(std::string token)
{
    string sGroup;
    SH::SplitToken(sGroup, token, "/");     // in case token is passed in as "group/key"
    string sKey;
    if (token.empty())
    {
        sKey = sGroup;
        if (gRegistry.contains(sKey))
            return gRegistry[sKey];

        return "";
    }

    string sValue;
    if (!gRegistry.Get(sGroup, sKey, sValue))
    {
        ZWARNING("Unable to lookup registry group:", sGroup, " key:", sKey);
    }

    return sValue;
}


ZWinPopupPanelBtn::ZWinPopupPanelBtn()
{
    mpWinPanel = nullptr;
    mPanelScaleVsBtn = { 2.0, 2.0 };
    mPanelPos = ZGUI::ePosition::ICOB;      // beneath
}

ZWinPopupPanelBtn::~ZWinPopupPanelBtn()
{
    Shutdown();
}

bool ZWinPopupPanelBtn::Init()
{
    return ZWinSizablePushBtn::Init();
}

bool ZWinPopupPanelBtn::Shutdown()
{
    mpWinPanel = nullptr;

    return ZWinSizablePushBtn::Shutdown();
}

bool ZWinPopupPanelBtn::OnMouseUpL(int64_t x, int64_t y)
{
    if (AmCapturing())
    {
        ReleaseCapture();
        if (x > 0 && x < mAreaInParent.Width() && y > 0 && y < mAreaInParent.Height())
        {
            TogglePanel();
        }

        mDrawState = kUp;
        Invalidate();

        return true;
    }

    return ZWin::OnMouseUpL(x, y);
}

void ZWinPopupPanelBtn::TogglePanel()
{
    ZWin* pTop = GetTopWindow();
    assert(pTop);
    assert(!mPanelLayout.empty());

    if (!mpWinPanel)
    {
        mpWinPanel = new ZWinPanel();
        mpWinPanel->mPanelLayout = mPanelLayout;
        pTop->ChildAdd(mpWinPanel, false);
    }

/*    ZRect rArea(mAreaInParent);
    rArea.left -= mAreaLocal.Width()/2;
    rArea.right += mAreaLocal.Height()/2;
    rArea.bottom += mAreaLocal.Height() * 3;
    rArea.OffsetRect(0, mAreaLocal.Height());

    mpWinPanel->SetRelativeArea(rArea, pTop->GetArea(), ZGUI::C);*/


//    mpWinPanel->SetRelativeArea(mPanelArea, pTop->GetArea(), ZGUI::C);
    UpdateUI();
    mpWinPanel->SetVisible(!mpWinPanel->mbVisible);
}

bool ZWinPopupPanelBtn::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    UpdateUI();
    ZWin::OnParentAreaChange();
}

void ZWinPopupPanelBtn::UpdateUI()
{
    if (mpWinPanel)
    {
        
        ZRect rArea(mAreaLocal.Width()*mPanelScaleVsBtn.x, mAreaLocal.Height()*mPanelScaleVsBtn.y);
        rArea.InflateRect(mBtnStyle.paddingH, mBtnStyle.paddingV);
        rArea = ZGUI::Arrange(rArea, mAreaAbsolute, mPanelPos, mBtnStyle.paddingH, mBtnStyle.paddingV);

        mpWinPanel->SetArea(rArea);
    }
}