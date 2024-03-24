#pragma once

#include "ZWin.h"
#include <string>
#include <unordered_set>
#include "ZMessageSystem.h"
#include "ZWinBtn.H"
#include "helpers/Registry.h"   // for resolving environment tokens like %apppath%


class ZXMLNode;

class RegisteredWin
{
public:
    RegisteredWin(int32_t _row = 0) : row(_row)
    {
    }
    // tbd....what attributes for this window?  sizing, grouping
    int32_t row;
};

typedef std::map<std::string, RegisteredWin> tRegisteredWins;

class ZWinPanel : public ZWin
{
public:

    enum eBehavior : uint32_t
    {
        kNone               = 0,
        kDrawBorder         = 1,        // 1
        kRelativeScale      = 1 << 1,   // 2
        kHideOnMouseExit    = 1 << 2,   // 4
        kHideOnButton       = 1 << 3,   // 8
        kCloseOnButton      = 1 << 4    // 16
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
   ZGUI::Style          mGroupingStyle;
   uint32_t             mBehavior;
   ZGUI::RelativeArea   mRelativeArea;

   inline bool          IsSet(uint32_t flag) { return (mBehavior & flag) != 0; }

   bool                 OnParentAreaChange();

protected:
   bool                 Paint();
   bool                 Process();

   bool                 Registered(const std::string& name);
   tWinList             GetRowWins(int32_t row);

   void                 UpdateUI();

   bool                 ParseLayout();
   bool                 ParseRow(ZXMLNode* pRow);
   bool                 ResolveLayoutTokens();
   std::string          ResolveToken(std::string token);

   std::string          AppendBehaviorMessages(const std::string& sMsg);

   tRegisteredWins      mRegistered;
   int32_t              mRows;
   int32_t              mSpacers;            // number of gSpacers between controls
   int32_t              mDrawBorder;
   bool                 mbMouseWasOver;
};


class ZWinPopupPanelBtn : public ZWinBtn
{
public:
    ZWinPopupPanelBtn();
    ~ZWinPopupPanelBtn();

    bool            Init();
    bool            Shutdown();
    bool            OnMouseUpL(int64_t x, int64_t y);
    bool            OnParentAreaChange();

    void            TogglePanel();

    std::string     mPanelLayout;
    ZGUI::ePosition mPanelPos;
    ZFPoint         mPanelScaleVsBtn;


protected:
    void            UpdateUI();

    ZWinPanel* mpWinPanel;
};
