#pragma once

#include "ZTypes.h"
#include "ZWin.h"

class ZWinImage;

/////////////////////////////////////////////////////////////////////////
// 
class ImageViewer : public ZWin
{
public:
    ImageViewer();
   
   bool		Init();
   bool     Process();
   bool		Paint();

   bool     ViewImage(const std::string& sFilename);

   virtual bool	OnKeyDown(uint32_t key);

protected:
    ZWinImage* mpWinImage;
    std::string msLoadedFilename;
    std::string msFilenameToLoad;
};
