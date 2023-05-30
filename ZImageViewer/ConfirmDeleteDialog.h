#pragma once

#include "ZWin.H"
#include <string>
#include "ZGUIStyle.h"
#include "ZWinFormattedText.h"
#include <list>
#include <filesystem>


class ConfirmDeleteDialog : public ZWinDialog
{
public:
    ConfirmDeleteDialog();
    bool        Init();
    virtual bool    OnKeyDown(uint32_t c);

    ZGUI::Style mStyle;

    std::string     msOnConfirmDelete;
    std::string     msOnGoBack;
    std::string     msOnCancel;

    static ConfirmDeleteDialog* ShowDialog(const std::string& sCaption, std::list<std::filesystem::path> fileList);


protected:
    void        OnConfirmDelete();
    void        OnGoBack();
    void        OnCancel();
    bool        HandleMessage(const ZMessage& message);

    bool        Paint();

    ZWinFormattedText* mpFilesList;
    std::string     msCaption;

    std::list<std::filesystem::path> mFiles;
};
