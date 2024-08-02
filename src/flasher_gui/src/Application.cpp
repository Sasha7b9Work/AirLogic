// 2022/04/29 13:56:38 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Application.h"
#include "Frame.h"


wxIMPLEMENT_APP(Application);


enum
{
    TIMER_ID = 111
};


bool Application::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    // we use a PNG image in our HTML page
    wxImage::AddHandler(new wxPNGHandler);

    // create and show the main application window
    Frame *frame = new Frame(_("AirLogic"));

    Init();

    Bind(wxEVT_TIMER, &Application::OnTimer, this, TIMER_ID);

    timer.SetOwner(this, TIMER_ID);

    timer.Start();

    frame->Show();

    return true;
}


void Application::OnTimer(wxTimerEvent &)
{
    Update();
}


int Application::OnExit()
{
    return wxApp::OnExit();
}


void Application::Init()
{

}


void Application::Update()
{

}
