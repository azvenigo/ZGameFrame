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
    mbAcceptsCursorMessages = true;
    mbMouseWasOver = false;
    mDrawBorder = false;
    mbConditionalVisible = false;
    mbSetVisibility = false;
    mSpacers = 0;        
}

ZWinPanel::~ZWinPanel()
{
}


bool ZWinPanel::Init()
{
    if (mbInitted)
        return true;

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
        ZRect rCaption(rBounds);

        rBounds.InflateRect(gDefaultGroupingStyle.pad.h, gDefaultGroupingStyle.pad.v);
        rBounds.right--;
        rBounds.bottom--;

        mpSurface->DrawRectAlpha(0x88000000, rBounds);
        rBounds.OffsetRect(1, 1);
        mpSurface->DrawRectAlpha(0x88ffffff, rBounds);



        rCaption.OffsetRect(gDefaultGroupingStyle.pad.h, -gDefaultGroupingStyle.fp.Height());
        gDefaultGroupingStyle.Font()->DrawTextParagraph(mpSurface.get(), group.first, rCaption);



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
            if (mbMouseWasOver && mbVisible)
            {
                if (!mAreaAbsolute.PtInRect(gInput.lastMouseMove))
                {
                    mbMouseWasOver = false;
                    mbConditionalVisible = false;
                    SetVisible(false);
                }
            }

            if (mAreaAbsolute.PtInRect(gInput.lastMouseMove))
            {
                mbMouseWasOver = true;
            }

            if (mbConditionalVisible != mbVisible)
                UpdateVisibility();
        }
    }

    if (IsSet(kShowOnTrigger))
    {
        if (gInput.captureWin == nullptr)
        {
            mbConditionalVisible = mrTrigger.PtInRect(gInput.lastMouseMove) || mAreaAbsolute.PtInRect(gInput.lastMouseMove);
            if (mbConditionalVisible != mbVisible)
                UpdateVisibility();
        }
    }

    return ZWin::Process();
}

void ZWinPanel::UpdateVisibility()
{
    bool bShouldBeVisible = mbConditionalVisible || mbSetVisibility;
    if (bShouldBeVisible != mbVisible)
    {
        if (mpParentWin)
            mpParentWin->InvalidateChildren();
        if (bShouldBeVisible)
            UpdateUI();

        if (bShouldBeVisible)
        {
            mIdleSleepMS = 25;
        }
        else
        {
            if (IsSet(kShowOnTrigger))
                mIdleSleepMS = 200;
            else
                mIdleSleepMS = 30000;
        }

    }

    ZWin::SetVisible(bShouldBeVisible);
}


bool ZWinPanel::ParseLayout()
{
	string sElement;

	ZXML tree;
    tree.Init(mPanelLayout);

    ZXML* pPanel = tree.GetChild("panel");
	if (!pPanel)
		return false;

	// Panel Attributes
    //      Behavior
    if (SH::ToBool(pPanel->GetAttribute("show_on_mouse_enter")))
        mBehavior |= kShowOnTrigger;


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
        mBehavior |= kRelativeArea;
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
        if (r.winname == _name)
            return true;
    }

    return false;
}

tRowElements ZWinPanel::GetRowElements(int32_t row, ZGUI::ePosition alignment)
{
    tRowElements wins;
    if (row >= 0 && row < mRows)
    {
        for (auto& r : mRegistered)
        {
            if (r.row == row)
            {
                if ((alignment & ZGUI::R) && ((r.style.pos & ZGUI::R) == 0))  // if asking for right alignment but element alignment isn't right aligned skip
                    continue;
                wins.push_back(r);
            }
        }
    }

    return wins;
}

string ZWinPanel::MakeButton(const string& id, const string& group, const string& caption, const string& svgpath, const string& tooltip, const string& message, float aspect, ZGUI::Style btnstyle, ZGUI::Style captionstyle)
{
    string s = "<button id=\"" + id + "\"";
    if (!group.empty())
        s += " group=\"" + group + "\"";
    if (!caption.empty())
        s += " caption=\"" + caption + "\" ";
    if (!svgpath.empty())
        s += " svgpath=\"" + svgpath + "\"";
    if (aspect != 1.0)
        s += " aspect=" + SH::FromDouble(aspect);
    if (!tooltip.empty())
        s += " tooltip=\"" + tooltip + "\"";
    if (!message.empty())
        s += " msg=\"" + ZXML::Encode(message) + "\"";
    if (!btnstyle.Uninitialized())
        s += " style=" + ZXML::Encode((string)btnstyle);
    if (!captionstyle.Uninitialized())
        s += " captionstyle=" + ZXML::Encode((string)captionstyle);

    s += "/>";

    return s;
}

string ZWinPanel::MakeSubmenu(const string& id, const string& group, const string& caption, const string& svgpath, const string& tooltip, const string& panellayout, ZGUI::RA_Descriptor rad, float aspect, ZGUI::Style style)
{
    string s = "<panelbutton id=\"" + id + "\"";
    if (!group.empty())
        s += " group=\"" + group + "\"";
    if (!caption.empty())
        s += " caption=\"" + caption + "\" ";
    if (!svgpath.empty())
        s += " svgpath=\"" + svgpath + "\"";
    if (aspect != 1.0)
        s += " aspect=" + SH::FromDouble(aspect);
    if (!tooltip.empty())
        s += " tooltip=\"" + tooltip + "\"";
    assert(!panellayout.empty());
    s += " layout=\"" + ZXML::Encode(panellayout) + "\"";
    s += " rad=\"" + ZXML::Encode((string) rad) + "\"";
    if (!style.Uninitialized())
        s += " style=" + ZXML::Encode((string)style);
    s += "/>";

    return s;
}

string ZWinPanel::MakeRadio(const string& id, const string& group, const string& radiogroup, const string& caption, const string& svgpath, const string& tooltip, const string& checkmessage, const string& uncheckmessage, float aspect, ZGUI::Style checkedStyle, ZGUI::Style uncheckedStyle)
{
    string s = "<radio id=\"" + id + "\"";
    if (!group.empty())
        s += " group=\"" + group + "\"";
    if (!radiogroup.empty())
        s += " radiogroup=\"" + radiogroup + "\"";
    if (!caption.empty())
        s += " caption=\"" + caption + "\" ";
    if (!svgpath.empty())
        s += " svgpath=\"" + svgpath + "\"";
    if (aspect != 1.0)
        s += " aspect=" + SH::FromDouble(aspect);
    if (!tooltip.empty())
        s += " tooltip=\"" + tooltip + "\"";
    if (!checkmessage.empty())
        s += " checkmsg=\"" + ZXML::Encode(checkmessage) + "\"";
    if (!uncheckmessage.empty())
        s += " uncheckmsg=\"" + ZXML::Encode(uncheckmessage) + "\"";
    if (!checkedStyle.Uninitialized())
        s += " checkedstyle=" + ZXML::Encode((string)checkedStyle);
    if (!uncheckedStyle.Uninitialized())
        s += " uncheckedstyle=" + ZXML::Encode((string)uncheckedStyle);

    s += "/>";

    return s;
}



bool ZWinPanel::ParseRow(ZXML* pRow)
{
	ZASSERT(pRow);
    tXMLNodeList controls;
    pRow->GetChildren(controls);

    // for each 
    for (auto control : controls)
    {

        string type = control->GetName();
        string id = control->GetAttribute("id");
        string groupname = control->GetAttribute("group");


        RowElement reg(id, groupname, mRows);
        if (control->HasAttribute("aspect"))
            reg.aspectratio = (float)SH::ToDouble(control->GetAttribute("aspect"));
        if (control->HasAttribute("style"))
            reg.style = ZGUI::Style(control->GetAttribute("style"));


        string msgSuffix;
        if (IsSet(kHideOnButton))
        {
            msgSuffix = "{set_visible;visible=0;target=" + GetTargetName() + "}";
        }
        else if (IsSet(kCloseOnButton))
        {
            msgSuffix = "{kill_child;name=" + msWinName + "}";
        }

        ZWin* pWin = nullptr;

        if (type == "panelbutton")
        {
            pWin = new ZWinPopupPanelBtn();
            ZWinPopupPanelBtn* pBtn = (ZWinPopupPanelBtn*)pWin;
            pBtn->mPanelLayout = control->GetAttribute("layout");
            pBtn->mSVGImage.imageFilename = ResolveTokens(control->GetAttribute("svgpath"));
//            assert(control->HasAttribute("rad"));
            if (control->HasAttribute("rad"))
                pBtn->panelRAD = control->GetAttribute("rad");
        }
        else if (type == "button")
        {
            // add a button
            pWin = new ZWinBtn();
            ZWinBtn* pBtn = (ZWinBtn*)pWin;

            if (control->HasAttribute("caption"))
            {
                pBtn->mCaption.sText = control->GetAttribute("caption");

                if (control->HasAttribute("captionstyle"))
                    pBtn->mCaption.style = ZGUI::Style(control->GetAttribute("captionstyle"));
            }
            if (control->HasAttribute("svgpath"))
            {
                pBtn->mSVGImage.imageFilename = ResolveTokens(control->GetAttribute("svgpath"));
            }

            pBtn->msButtonMessage = control->GetAttribute("msg") + msgSuffix;
        }
        else if (type == "radio")
        {
            // add radio button
            pWin = new ZWinCheck();
            ZWinCheck* pCheck = (ZWinCheck*)pWin;
            pCheck->msButtonMessage = control->GetAttribute("checkmsg") + msgSuffix;
            pCheck->msUncheckMessage = control->GetAttribute("uncheckmsg") + msgSuffix;
            pCheck->msRadioGroup = control->GetAttribute("radiogroup") + msgSuffix;

            if (control->HasAttribute("checkedstyle"))
                pCheck->mCheckedStyle = ZGUI::Style(control->GetAttribute("checkedstyle"));
            if (control->HasAttribute("uncheckedstyle"))
                pCheck->mUncheckedStyle = ZGUI::Style(control->GetAttribute("uncheckedstyle"));


        }
        else if (type == "label")
        {
            // add label
            pWin = new ZWinLabel();
            ZWinLabel* pLabel = (ZWinLabel*)pWin;
            pLabel->msText = control->GetAttribute("caption");
            pLabel->mStyle.pos = ZGUI::Fit;
        }
        else if (type == "space")
        {
        }
        else
        {
            ZASSERT(false);
            ZERROR("Unsupported type:", type);
            return false;
        }

        // Common attributes for windows
        if (pWin)
        {
            if (id.empty() || Registered(id))
            {
                ZERROR("ID:", id, " invalid. Must have unique IDs for each registered element.");
                assert(false);
                return false;
            }


            pWin->msWinGroup = reg.groupname;
            pWin->msTooltip = control->GetAttribute("tooltip");
            pWin->msWinName = id;
            ChildAdd(pWin);

            // if any windows are part of draw groups, enable the flag
            if (!pWin->msWinGroup.empty())
            {
                mBehavior |= kDrawGroupFrames;
            }
        }


        mRegistered.emplace_back(std::move(reg));
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
        gMessageSystem.Post(ZMessage("kill_child", "name", msWinName));
        return true;
    }

	return ZWin::HandleMessage(message);
}

void ZWinPanel::UpdateUI()
{
    ZRect ref = mAreaLocal;
    ZGUI::Style style = mStyle;

    if (mBehavior&kRelativeArea)
    {
        assert(!mRAD.referenceWindow.empty());

        if (mRAD.referenceWindow == "full")
            ref = grFullArea;
        else
        {
            ZWin* pWin = GetTopWindow()->GetChildWindowByWinNameRecursive(mRAD.referenceWindow);
            assert(pWin);
            if (pWin)
            {
                ref = pWin->mAreaAbsolute;
            }
        }

        SetArea(ZGUI::Align(mAreaAbsolute, ref, (ZGUI::ePosition)mRAD.alignment, mStyle.pad.h, mStyle.pad.v, mRAD.wRatio, mRAD.hRatio, mRAD.minWidth, mRAD.minHeight, mRAD.maxWidth, mRAD.maxHeight));
        ref = mAreaLocal;
    }

    if (mRows > 0 && ref.Area())
    {
        ZRect controlArea = ref;

//        if (mDrawBorder)
//            controlArea.DeflateRect(mStyle.pad.h, mStyle.pad.v);

        int64_t spaceBetweenRows = mSpacers * mStyle.pad.v;
        int64_t totalSpaceBetweenRows = (mRows - 1) * spaceBetweenRows;

        int64_t rowh = (controlArea.Height()-totalSpaceBetweenRows)/mRows;

        for (int32_t row = 0; row < mRows; row++)
        {
            ZRect rRowArea(controlArea.left, controlArea.top + row * (rowh + spaceBetweenRows), controlArea.right, controlArea.top + (row + 1) * (rowh + spaceBetweenRows));

            if (IsSet(kDrawGroupFrames))
            {
                rRowArea.DeflateRect(gDefaultGroupingStyle.pad.h * 2, gDefaultGroupingStyle.pad.v * 2);
            }

            tRowElements leftElements = GetRowElements(row, ZGUI::L);
            tRowElements rightElements = GetRowElements(row, ZGUI::R);

            int64_t elementMaxWidth = rRowArea.Width() / (leftElements.size() + rightElements.size());

            if (!leftElements.empty())
            {
                string lastSeenGroupName;
                for (int i = 0; i < leftElements.size(); i++)
                {
                    RowElement& element = leftElements[i];
                    
                    bool bAddSpaceBetweenGroups = (i > 0) && leftElements[i - 1].groupname != element.groupname;    // if group name has changed from last element to current, add gap
                    if (bAddSpaceBetweenGroups)
                        rRowArea.left += gDefaultGroupingStyle.pad.h * 4;

                    assert(rRowArea.Width() > 0);
                    int64_t h = rRowArea.Height();
                    int64_t w = min<int64_t>(elementMaxWidth, (int64_t)((float)h * element.aspectratio));
                    int64_t x = rRowArea.left;
                    int64_t y = rRowArea.top;

                    ZRect r(x, y, x + w, y + h);
                    if (!element.winname.empty())
                    {
                        ZWin* pWin = GetChildWindowByWinName(element.winname);
                        if (pWin)
                            pWin->SetArea(r);
                    }

                    rRowArea.left += w + mStyle.pad.h;
                }
            }


            if (!rightElements.empty())
            {
                // walk over the rows backward for any windows right aligned
                for (int i = (int)rightElements.size()-1; i >= 0; i--)
                {
                    RowElement& element = rightElements[i];

                    bool bAddSpaceBetweenGroups = (i < (rightElements.size()-1) && (leftElements[i + 1].groupname != element.groupname));    // if group name has changed from last element to current, add gap
                    if (bAddSpaceBetweenGroups)
                        rRowArea.left -= gDefaultGroupingStyle.pad.h * 4;
#ifdef _DEBUG
                    if (rRowArea.Width() <= 0)
                    {
                        string sDebugLayout = SH::URL_Decode(mPanelLayout);
                        int stophere = 5;
                    }
#endif
                    assert(rRowArea.Width() > 0);
                    int64_t h = rRowArea.Height();
                    int64_t w = min<int64_t>(elementMaxWidth, (int64_t)((float)h * element.aspectratio));
                    int64_t x = rRowArea.right - mStyle.pad.h - w;
                    int64_t y = rRowArea.top;

                    ZRect r(x, y, x + w, y + h);
                    ZWin* pWin = GetChildWindowByWinName(element.winname);
                    if (pWin)
                        pWin->SetArea(r);

                    rRowArea.right -= (w + mStyle.pad.h);
                }
            }
        }
    }
}

void ZWinPanel::SetRelativeArea(const ZGUI::RA_Descriptor& rad)
{
//    assert(IsSet(kRelativeScale));
//    mRelativeArea = ZGUI::RelativeArea(rad);
//    SetArea(rad.area);

    mRAD = rad;
    UpdateUI();
}


bool ZWinPanel::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

    ZWin::OnParentAreaChange();
    UpdateUI();

    return true;
}

void ZWinPanel::SetVisible(bool bVisible)
{
    mbSetVisibility = bVisible;
    UpdateVisibility();
}

string ZWinPanel::ResolveTokens(std::string full)
{
    for (size_t i = 0; i < full.size(); i++)
    {
        if (full[i] == '\'' || full[i] == '\"')
        {
            i = SH::FindMatching(mPanelLayout, i);  // skip enclosed
        }
        else if (full[i] == '$')
        {
            for (size_t j = i+1; j < full.size(); j++)
            {
                if (full[j] == '$')
                {
                    string sToken = full.substr(i+1, j - i - 1);    // pull token name
                    if (!sToken.empty())
                    {
                        string sValue = ResolveToken(sToken);
                        if (!sValue.empty())
                        {
                            string sNew = full.substr(0, i);
                            sNew += sValue;
                            sNew += full.substr(j + 1);
                            full = sNew;
                            i += sValue.length() - sToken.length() + 2;         // adjust the iterator to skip the replaced token. +2 to skip the leading and end '%'
                            break;
                        }
                    }
                }
            }
        }
    }

    return full;
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
    panelRAD = gDefaultRAD;
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
        mpWinPanel->mRAD = panelRAD;
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
        ZWin* pWin = GetTopWindow()->GetChildWindowByWinNameRecursive(panelRAD.referenceWindow);
        assert(pWin);
        if (pWin)
        {
            ZRect ref = pWin->mAreaAbsolute;
            ZRect r = ZGUI::Align(mpWinPanel->mAreaAbsolute, ref, (ZGUI::ePosition)panelRAD.alignment, 0, 0, panelRAD.wRatio, panelRAD.hRatio);
            mpWinPanel->SetArea(r);
        }
    }
}
