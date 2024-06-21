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

namespace ZImageViewer
{
    void InitControlPanel();
    void DeleteAllButControlPanelAndDebugConsole();
    void InitChildWindows();
    void InitializeFonts();
};


namespace ZFrameworkApp
{
    bool InitRegistry(std::filesystem::path userDataPath);  // pre-init
    bool Initialize(int argc, char* argv[], std::filesystem::path userDataPath);
    void Shutdown();
};