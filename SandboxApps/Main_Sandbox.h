#pragma once

#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "ZWin.h"
#include "ZMainWin.h"
#include "ZTimer.h"
#include "ZFont.h"

#include "ZMessageSystem.h"
#include "ZTickManager.h"
#include "ZStringHelpers.h"
#include "ZStdDebug.h"
#include "ZXMLNode.h"
#include "ZRasterizer.h"
#include "ZPreferences.h"
#include "ZAnimator.h"
#include "FloatLinesWin.h"
#include "ZScriptedDialogWin.h"
#include "Resources.h"
#include "ProcessImageWin.h"
#include <string>
#include "ZStringHelpers.h"
#include <random>
#include "LifeWin.h"
#include "ZImageWin.h"
#include "ZSliderWin.h"
#include "ZWinControlPanel.h"
#include "ZRandom.h"
#include "TextTestWin.h"
#include "Marquee.h"

using namespace std;

extern ZRect                grFullArea;
extern ZMainWin*            gpMainWin;
extern ZWinControlPanel*    gpControlPanel;
extern ZGraphicSystem       gGraphicSystem;
extern ZGraphicSystem*      gpGraphicSystem;
extern ZRasterizer          gRasterizer;
extern bool                 gbGraphicSystemResetNeeded;
extern bool                 gbApplicationExiting;
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

extern list<int32_t>        frameworkFontSizes;
extern list<string>         frameworkFontNames;
extern string               gsDefaultFontName;


namespace Sandbox
{
    enum eSandboxMode
    {
        kFloatLinesWin = 1,
        kCheckerboard = 2,
        kRandomWindows = 3,
        kImageProcess = 4,
        kOneLifeWin = 5,
        kTextTestWin = 6,
        kMarquee = 7,
    };

    void SandboxShutdown();
    void InitControlPanel();
    void SandboxDeleteAllButControlPanel();
    void SandboxInitChildWindows(eSandboxMode mode);
    void SandboxInitializeFonts();
    bool SandboxInitialize();
    void SandboxShutdown();
};