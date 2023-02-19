#ifndef LIFEWIN_H
#define LIFEWIN_H

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"

/////////////////////////////////////////////////////////////////////////
// 
class cLifeWin : public ZWin
{
public:
   cLifeWin();
   
   bool		Init();
   bool		Shutdown();
   void		SetGridSize(int64_t nWidth, int64_t nHeight) { mnWidth = nWidth; mnHeight = nHeight; }
   bool		OnMouseDownL(int64_t x, int64_t y);
   bool		OnMouseUpL(int64_t x, int64_t y);
   bool		OnMouseMove(int64_t x, int64_t y);
   bool		OnMouseDownR(int64_t x, int64_t y);
   bool     Process();
   bool		Paint();
   
   //bool		OnChar(char key);
   virtual bool	OnKeyDown(uint32_t key);

   void		Load(std::string sName);
   void		Save(std::string sName);
   
private:
   void		PaintGrid();
   
   tZBufferPtr     pBufBackground;
   tZFontPtr    mpFont;
   bool		    mbForward;

   tZBufferPtr		mpGrid;
   tZBufferPtr		mpGrid2;


   tZBufferPtr	    mpCurGrid;
   int64_t	    mnIterations;
   ZTimer	    mTimer;

   int64_t	    mnWidth;
   int64_t	    mnHeight;
   bool		    mbPaused;
   int64_t	    mnNumCells;

   uint32_t     mnOffPixelCol;
   uint32_t     mnOnPixelCol;

   bool		    mbPainting;
   bool		    mbDrawing;
};


#endif