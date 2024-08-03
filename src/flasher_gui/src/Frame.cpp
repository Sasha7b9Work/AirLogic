// 2022/04/29 13:56:48 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Frame.h"
#include "TableModules.h"


Frame *Frame::self = nullptr;


enum
{
    FILE_QUIT = wxID_HIGHEST + 1,

    TOOL_OPEN,
    TOOL_SAVE,
    TOOL_NEW,

    TOOL_UNDO,
    TOOL_REDO,

    TOOL_VIEW_BRIEF,        // Сокращённый вид отображения
    TOOL_VIEW_FULL,         // Полный вид отображения

    TOOL_CONSOLE,
    TOOL_DATABASE,

    MEAS_PRESSURE,          // Давление
    MEAS_ILLUMINATION,      // Освещённость
    MEAS_HUMIDITY,          // Влажность
    MEAS_VELOCITY,          // Скорость
    MEAS_TEMPERATURE,       // Температура

    ID_SPEED_1,
    ID_SPEED_2,
    ID_SPEED_5,
    ID_SPEED_30,
    ID_SPEED_60,

    ID_MODE_VIEW_FULL,
    ID_MODE_VIEW_TABLE,
    ID_MODE_VIEW_GRAPH
};


Frame::Frame(const wxString &title)
    : wxFrame((wxFrame *)NULL, wxID_ANY, title)
{
    self = this;

    SetIcon(wxICON(MAIN_ICON));

    Bind(wxEVT_MENU, &Frame::OnMenuSettings, this);

    Bind(wxEVT_MENU, &Frame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &Frame::OnMenuTool, this, TOOL_CONSOLE);
    Bind(wxEVT_MENU, &Frame::OnMenuTool, this, TOOL_DATABASE);
    Bind(wxEVT_CLOSE_WINDOW, &Frame::OnCloseWindow, this);

    Bind(wxEVT_SIZE, &Frame::OnSize, this);

    CreatePanel();

    SetClientSize(470, 215);
}


void Frame::OnMenuSettings(wxCommandEvent &event)
{
    int id = event.GetId();

    if (id >= ID_SPEED_1 && id <= ID_SPEED_60)
    {
        static const int scales[] = { 1, 2, 5, 30, 60 };
    }
    else if (id >= ID_MODE_VIEW_FULL && id <= ID_MODE_VIEW_GRAPH)
    {
    }
}


void Frame::OnSize(wxSizeEvent &event)
{
    Layout();

    event.Skip();
}


void Frame::OnAbout(wxCommandEvent &WXUNUSED(event))
{
    wxBoxSizer *topsizer;
    wxDialog dlg(this, wxID_ANY, wxString(_("About")));

    topsizer = new wxBoxSizer(wxVERTICAL);

    wxButton *bu1 = new wxButton(&dlg, wxID_OK, _("OK"));
    bu1->SetDefault();

    topsizer->Add(bu1, 0, wxALL | wxALIGN_RIGHT, 15);

    dlg.SetSizer(topsizer);
    topsizer->Fit(&dlg);

    dlg.ShowModal();
}


void Frame::OnMenuTool(wxCommandEvent &event)
{
    int id = event.GetId();

    if (id == TOOL_CONSOLE)
    {
    }
    else if (id == TOOL_DATABASE)
    {

    }
}


void Frame::OnCloseWindow(wxCloseEvent &event)
{
    event.Skip();
}


void Frame::CreatePanel()
{
    wxWindow *window = new wxWindow(this, wxID_ANY, { 500, 500 });

    wxBoxSizer *top = new wxBoxSizer(wxHORIZONTAL);
    {
        wxStaticText *txtDevice = new wxStaticText(window, wxID_ANY, _("Устройство"));

        wxStaticText *txtConnect = new wxStaticText(window, wxID_ANY, _("Подключено"));

        wxButton *btnConfig = new wxButton(window, wxID_ANY, _("Конфигурация"));

        top->AddSpacer(50);
        top->Add(txtDevice);
        top->AddSpacer(50);
        top->Add(txtConnect);
        top->AddSpacer(100);
        top->Add(btnConfig);
    }

    wxBoxSizer *medium = new wxBoxSizer(wxHORIZONTAL);
    {
        new TableModules(window);

        TableModules::self->TestView();

        medium->Add(TableModules::self);
    }

    wxBoxSizer *down = new wxBoxSizer(wxHORIZONTAL);
    {
        wxButton *btnUpdate = new wxButton(window, wxID_ANY, _("Обновить"));

        down->AddSpacer(200);
        down->Add(btnUpdate);
    }

    wxBoxSizer *window_sizer = new wxBoxSizer(wxVERTICAL);

    window_sizer->AddSpacer(10);
    window_sizer->Add(top);
    window_sizer->AddSpacer(10);
    window_sizer->Add(medium);
    window_sizer->AddSpacer(10);
    window_sizer->Add(down);
    window_sizer->AddSpacer(10);

    window->SetSizer(window_sizer);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(window);

    SetSizer(sizer);
}
