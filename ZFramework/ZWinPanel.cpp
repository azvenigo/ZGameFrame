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
    if (msWinName.empty())
        msWinName = "ZWinPanel_" + gMessageSystem.GenerateUniqueTargetName();

    mStyle = gDefaultPanelStyle;
    mbAcceptsCursorMessages = true;
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

	return ZWin::Paint();
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

    //      Sizing
    //          min size
    //          max size
    //          % of parent or % of full screen, or?

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

        string msgSuffix;
        if ((mBehavior & kCloseOnButton) != 0)
            msgSuffix = ";closepanel&target=" + GetTargetName();

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
            pBtn->msButtonMessage = control->GetAttribute("msg") + msgSuffix;
        }
        else if (type == "svgbutton")
        {
            // add svg button
            pWin = new ZWinSizablePushBtn();
            ZWinSizablePushBtn* pBtn = (ZWinSizablePushBtn*)pWin;
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
            pLabel->msText = control->GetAttribute("caption") + msgSuffix;
        }
        else
        {
            ZASSERT(false);
            ZERROR("Unsupported type:", type);
            return false;
        }

        // Common attributes
        pWin->msWinGroup = control->GetAttribute("group");
        pWin->msWinName = id;
        ChildAdd(pWin);

        mRegistered[id].row = mRows;
        return true;
    }

	return false;
}

bool ZWinPanel::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "closepanel")
    {
        if (mpParentWin)
            mpParentWin->SetFocus();
        gMessageSystem.Post("kill_child", "name", msWinName);
    }

	return ZWin::HandleMessage(message);
}

void ZWinPanel::UpdateUI()
{
    ZRect rLocal = mRelativeArea.Area(grFullArea);

    int32_t h = mAreaLocal.Height() - (mRows + 1) * mStyle.paddingV;
    for (int32_t i = 0; i < mRows; i++)
    {
        tWinList rowWins = GetRowWins(i);
        int32_t w = mAreaLocal.Width() - (rowWins.size() + 1) * mStyle.paddingH;

        int rowi = 0;
        for (auto pWin : rowWins)
        {
            ZPoint tl(mStyle.paddingH + rowi * w, mStyle.paddingV);
            ZRect r(tl.x, tl.y, tl.x + w, tl.y + h);
            pWin->SetArea(r);
        }
    }
}

void ZWinPanel::SetRelativeArea(const ZRect& ref, ZGUI::ePosition pos, const ZRect& area)
{
    mRelativeArea = ZGUI::RelativeArea(area, ref, pos);
    SetArea(area);
}


bool ZWinPanel::OnParentAreaChange()
{
    if (!mpSurface)
        return false;

//    SetArea(mpParentWin->GetArea());

    ZRect rNewParent(mpParentWin->GetArea());

    SetArea(mRelativeArea.Area(rNewParent));


    ZWin::OnParentAreaChange();
    UpdateUI();

    return true;
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

