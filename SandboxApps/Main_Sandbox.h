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
        k3DTestWin = 7,
        kChessWin = 8,
        kTestWin = 9,
        kParticleSandbox = 10,
        kOnePageDoc = 11
    };

    void InitControlPanel();
    void DeleteAllButControlPanelAndDebugConsole();
    void InitChildWindows(eSandboxMode mode);

    tWinList sandboxWins;
};

namespace ZFrameworkApp
{
    bool InitRegistry(std::filesystem::path userDataPath);
    bool Initialize(int argc, char* argv[], std::filesystem::path userDataPath);
    void Shutdown();
};