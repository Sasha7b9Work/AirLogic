// 2022/04/29 13:56:55 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


class DiagramPool;


class Frame : public wxFrame
{
public:
    Frame(const wxString &title);

    void OnAbout(wxCommandEvent &event);

    static Frame *self;

private:

    wxToolBar *toolBar = nullptr;

    void OnSize(wxSizeEvent &);

    void OnCloseWindow(wxCloseEvent &);

    void OnMenuTool(wxCommandEvent &);

    void OnMenuSettings(wxCommandEvent &);

    void CreatePanel();
};
