// 2024/08/02 20:52:35 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


struct TypeModule
{
    enum E
    {
        Controller,
        Display,
        Keyboard,
        Modem,
        Count
    };

    static wxString Name(E);
};


class TableModules : public wxListView
{
public:
    TableModules(wxWindow *);

    static TableModules *self;

    void AppendModule(TypeModule::E);
    void SetVersion(TypeModule::E, const wxString &);
    void SetFile(TypeModule::E, const wxString &);
    void SetStatus(TypeModule::E, const wxString &);

    void TestView();

private:
    std::vector<TypeModule::E> lines;       // Номера строк для модулей

    long GetNumLine(TypeModule::E) const;
};
