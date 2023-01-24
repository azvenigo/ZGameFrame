#pragma once

#include "ZStdTypes.h"
#include "ZWin.h"
#include "ZFont.h"

/////////////////////////////////////////////////////////////////////////
// 
class ZChessWin : public ZWin
{
public:
    ZChessWin();
   
    bool    Init();
    bool    Shutdown();
    bool    OnMouseDownL(int64_t x, int64_t y);
    bool    OnMouseUpL(int64_t x, int64_t y);
    bool    OnMouseMove(int64_t x, int64_t y);
    bool    OnMouseDownR(int64_t x, int64_t y);
    bool    Process();
    bool    Paint();
    virtual bool	HandleMessage(const ZMessage& message);

    bool    OnChar(char key);

   
private:
    void    DrawBoard();

    void    UpdateSize();


   ZDynamicFont*    mpSymbolicFont;

   int64_t         mnPieceHeight;
   bool             mbOutline;

   char     mBoard[8][8] =
   { {'R','N','B','Q','K','B','N','R'},
     {'P','P','P','P','P','P','P','P'},
     {0},
     {0},
     {0},
     {0},
     {'p','p','p','p','p','p','p','p'},
     {'r','n','b','q','k','b','n','r'} };


};
