#pragma once

#include "ZWin.h"
#include <string>
#include "ZFont.h"
#include <list>
#include "ZMessageSystem.h"


class ZWinSizablePushBtn;
class ZXMLNode;

typedef  std::list<ZWinSizablePushBtn*> tMessageBoxButtonList;

class ZScriptedDialogWin : public ZWin
{
public:
   ZScriptedDialogWin();
   ~ZScriptedDialogWin();

   void PreInit(const std::string& sDialogScript);
   virtual bool Init();
   virtual bool Shutdown();

   // IMessageTarget
   virtual bool HandleMessage(const ZMessage& message);

protected:
   virtual bool Paint();

private:

   bool ExecuteScript(std::string sDialogScript);
   bool ProcessNode(ZXMLNode* pNode);

   std::string				msDialogScript;
   tMessageBoxButtonList	mArrangedMessageBoxButtonList;
   bool						mbDrawDefaultBackground;
   bool						mbFillBackground;
   uint32_t					mnBackgroundColor;
};

