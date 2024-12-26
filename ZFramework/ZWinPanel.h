#pragma once

#include "ZWin.h"
#include <string>
#include <unordered_set>
#include "ZMessageSystem.h"
#include "ZWinBtn.H"
#include "helpers/Registry.h"   // for resolving environment tokens like $apppath$


class ZXML;

class RowElement
{
public:
    RowElement(const std::string& _winname, const std::string& _groupname, int32_t _row = 0, ZGUI::Style _style = {}, float _aspectratio = 1.0) : winname(_winname), groupname(_groupname), row(_row), style(_style), aspectratio(_aspectratio)
    {
    }
    std::string     winname;    
    std::string     groupname; 
    int32_t         row;
    float           aspectratio; 
    ZGUI::Style     style;
};

typedef std::vector<RowElement> tRowElements;

class ZWinPanel : public ZWin
{
public:

    enum eBehavior : uint32_t
    {
        kNone               = 0,
        kDrawBorder         = 1,        // 1
        kRelativeArea       = 1 << 1,   // 2
        kShowOnTrigger      = 1 << 2,   // 4
        kHideOnMouseExit    = 1 << 3,   // 8
        kHideOnButton       = 1 << 4,   // 16
        kCloseOnButton      = 1 << 5,   // 32
        kDrawGroupFrames    = 1 << 6,   // 64
    };

    ZWinPanel();
    ~ZWinPanel();

   bool                 Init(); 
   bool                 Shutdown();
   void                 SetVisible(bool bVisible = true);

   bool                 HandleMessage(const ZMessage& message);

   void                 SetRelativeArea(const ZGUI::RA_Descriptor& rad); // sets up relative area to automatically adjust when parent area changes


   std::string          mPanelLayout;
   ZGUI::Style          mStyle;
   uint32_t             mBehavior;
   ZGUI::RA_Descriptor  mRAD;
   ZRect                mrTrigger;

   inline bool          IsSet(uint32_t flag) { return (mBehavior & flag) != 0; }

   static std::string   MakeButton(const std::string& id, const std::string& group, const std::string& caption, const std::string& svgpath, const std::string& tooltip, const std::string& message, float aspect, ZGUI::Style btnstyle, ZGUI::Style captionstyle);
   static std::string   MakeSubmenu(const std::string& id, const std::string& group, const std::string& caption, const std::string& svgpath, const std::string& tooltip, const std::string& panellayout, ZGUI::RA_Descriptor rad, float aspect, ZGUI::Style style);
   static std::string   MakeRadio(const std::string& id, const std::string& group, const std::string& radiogroup, const std::string& caption, const std::string& svgpath, const std::string& tooltip, const std::string& checkmessage, const std::string& uncheckmessage, float aspect, ZGUI::Style checkedStyle, ZGUI::Style uncheckedStyle);

   bool                 OnParentAreaChange();

protected:
   bool                 Paint();
   bool                 Process();

   bool                 Registered(const std::string& name);
   tRowElements         GetRowElements(int32_t row, ZGUI::ePosition alignment = ZGUI::Unknown);

   void                 UpdateUI();
   void                 UpdateVisibility();

   bool                 ParseLayout();
   bool                 ParseRow(ZXML* pRow);
   std::string          ResolveTokens(std::string full);
   std::string          ResolveToken(std::string token);

   std::string          AppendBehaviorMessages(const std::string& sMsg);

   tRowElements         mRegistered;
   int32_t              mRows;
   int32_t              mSpacers;            // number of gSpacers between controls
   int32_t              mDrawBorder;
   bool                 mbMouseWasOver;
   bool                 mbSetVisibility;
   bool                 mbConditionalVisible;
};


class ZWinPopupPanelBtn : public ZWinBtn
{
public:
    ZWinPopupPanelBtn();
    ~ZWinPopupPanelBtn();

    bool                Init();
    bool                Shutdown();
    bool                OnMouseUpL(int64_t x, int64_t y);
    bool                OnParentAreaChange();

    void                TogglePanel();

    std::string         mPanelLayout;
    ZGUI::RA_Descriptor panelRAD;

protected:
    void                UpdateUI();

    ZWinPanel* mpWinPanel;
};
