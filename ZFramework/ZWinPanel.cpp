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
    else if (SH::ToBool(pPanel->GetAttribute("close_on_button")))
        mBehavior |= kCloseOnButton;

    if (pPanel->HasAttribute("spacers"))
        mSpacers = (int32_t)SH::ToInt(pPanel->GetAttribute("spacers"));
    mDrawBorder = SH::ToBool(pPanel->GetAttribute("border"));


    if (pPanel->HasAttribute("style"))
        mStyle = ZGUI::Style(pPanel->GetAttribute("style"));

    if (pPanel->HasAttribute("rel_area_desc"))
    {
        ZGUI::RA_Descriptor rad(pPanel->GetAttribute("rel_area_desc"));
        mBehavior |= kRelativeScale;
        SetRelativeArea(rad);
    }



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

        string msgSuffix;
        if (IsSet(kHideOnButton))
        {
            msgSuffix = "{set_visible;visible=0;target=" + GetTargetName() + "}";
        }
        else if (IsSet(kCloseOnButton))
        {
            msgSuffix = "{kill_child;name=" + msWinName + "}";
        }

        if (id.empty() || Registered(id))
        {
            ZERROR("ID:", id, " invalid. Must have unique IDs for each registered window.");
            return false;
        }

        ZWin* pWin = nullptr;

        if (type == "panelbutton")
        {
            pWin = new ZWinPopupPanelBtn();
            ZWinPopupPanelBtn* pBtn = (ZWinPopupPanelBtn*)pWin;
            pBtn->mPanelLayout = control->GetAttribute("layout");
            pBtn->mSVGImage.Load(control->GetAttribute("svgpath"));
            if (control->HasAttribute("scale"))
                pBtn->mPanelScaleVsBtn = StringToFPoint(control->GetAttribute("scale"));
            if (control->HasAttribute("pos"))
                pBtn->mPanelPos = ZGUI::FromString(control->GetAttribute("pos"));

            assert(ZGUI::ValidPos(pBtn->mPanelPos));

        }
        else if (type == "button")
        {
            // add a button
            pWin = new ZWinBtn();
            ZWinBtn* pBtn = (ZWinBtn*)pWin;
            pBtn->mCaption.sText = control->GetAttribute("caption");
            pBtn->mCaption.autoSizeFont = true;
            pBtn->msButtonMessage = control->GetAttribute("msg") + msgSuffix;
        }
        else if (type == "svgbutton")
        {
            // add svg button
            pWin = new ZWinBtn();
            ZWinBtn* pBtn = (ZWinBtn*)pWin;
            pBtn->mSVGImage.Load(control->GetAttribute("svgpath"));
            pBtn->msButtonMessage = control->GetAttribute("msg") + msgSuffix;
        }
        else if (type == "toggle")
        {
            // add toggle
            pWin = new ZWinCheck();
            ZWinCheck* pCheck = (ZWinCheck*)pWin;
            pCheck->msButtonMessage = control->GetAttribute("checkmsg") + msgSuffix;
            pCheck->msUncheckMessage = control->GetAttribute("uncheckmsg") + msgSuffix;
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
        gMessageSystem.Post("{kill_child}", "name", msWinName);
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

        int64_t spaceBetweenRows = mSpacers * gSpacer;
        int64_t totalSpaceBetweenRows = (mRows - 1) * spaceBetweenRows;

        int64_t rowh = (controlArea.Height()-totalSpaceBetweenRows)/mRows;

        for (int32_t row = 0; row < mRows; row++)
        {
            tWinList rowWins = GetRowWins(row);
            if (!rowWins.empty())
            {
                int64_t w = controlArea.Width() / rowWins.size();

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

void ZWinPanel::SetRelativeArea(const ZGUI::RA_Descriptor& rad)
{
    assert(IsSet(kRelativeScale));
    mRelativeArea = ZGUI::RelativeArea(rad);
    SetArea(rad.area);
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
        if (mPanelLayout[i] == '\'' || mPanelLayout[i] == '\"')
        {
            i = SH::FindMatching(mPanelLayout, i);  // skip enclosed
        }
        else if (mPanelLayout[i] == '%')
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
    return ZWinBtn::Init();
}

bool ZWinPopupPanelBtn::Shutdown()
{
    mpWinPanel = nullptr;

    return ZWinBtn::Shutdown();
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
        assert(!mpWinPanel->IsSet(ZWinPanel::kCloseOnButton));
    }

    UpdateUI();
    mpWinPanel->SetVisible(!mpWinPanel->mbVisible);
}

bool ZWinPopupPanelBtn::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    UpdateUI();
    return ZWin::OnParentAreaChange();
}

void ZWinPopupPanelBtn::UpdateUI()
{
    if (mpWinPanel)
    {
        ZRect rArea((int64_t)(mAreaLocal.Width()*mPanelScaleVsBtn.x), (int64_t)(mAreaLocal.Height()*mPanelScaleVsBtn.y));
        rArea.InflateRect(mBtnStyle.paddingH, mBtnStyle.paddingV);
        assert(ZGUI::ValidPos(mPanelPos));
        rArea = ZGUI::Arrange(rArea, mAreaAbsolute, mPanelPos, mBtnStyle.paddingH, mBtnStyle.paddingV);

        mpWinPanel->SetArea(rArea);
    }
}
