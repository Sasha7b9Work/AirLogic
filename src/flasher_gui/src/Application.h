// 2022/04/29 13:56:43 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


class Application : public wxApp
{
public:
    virtual bool OnInit() wxOVERRIDE;

    void OnTimer(wxTimerEvent &);

private:

    wxTimer timer;

    void Init();

    void Update();

    virtual int OnExit() wxOVERRIDE;
};
