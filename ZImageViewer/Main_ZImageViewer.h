#pragma once

#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "ZWin.h"
#include "ZMainWin.h"
#include "ZTimer.h"
#include "ZFont.h"

#include "ZMessageSystem.h"
#include "ZTickManager.h"
#include "ZRasterizer.h"
#include "ZPreferences.h"
#include "ZAnimator.h"
#include "ZRandom.h"
#include "ZWinControlPanel.h"
#include "ZWinDebugConsole.h"


/*extern ZRect                grFullArea;
extern ZWinControlPanel*    gpControlPanel;
extern ZWinDebugConsole*    gpDebugConsole;
extern ZGraphicSystem       gGraphicSystem;
extern ZGraphicSystem*      gpGraphicSystem;
extern ZRasterizer          gRasterizer;
extern bool                 gbGraphicSystemResetNeeded;
extern bool                 gbPaused;
extern ZTimer               gTimer;
extern int64_t              gnFramesPerSecond;
extern ZFontSystem*         gpFontSystem;
extern std::list<string>    gDebugOutQueue;
extern std::mutex           gDebugOutMutex;
extern ZMessageSystem       gMessageSystem;
extern ZTickManager         gTickManager;
extern ZAnimator            gAnimator;
extern int64_t              gnCheckerWindowCount;
extern int64_t              gnRandomWindowCount;
extern int64_t              gnLifeGridSize;
extern ZWin*                gpCaptureWin;
extern ZWin*                gpMouseOverWin;
extern ZPoint               gLastMouseMove;
*/

namespace ZImageViewer
{
    void InitControlPanel();
    void DeleteAllButControlPanelAndDebugConsole();
    void InitChildWindows();
    void InitializeFonts();
};


namespace ZFrameworkApp
{
    bool Initialize(int argc, char* argv[], std::filesystem::path userDataPath);
    void Shutdown();
};