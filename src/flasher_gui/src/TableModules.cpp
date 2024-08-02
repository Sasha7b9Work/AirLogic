// 2024/08/02 20:52:58 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "TableModules.h"


TableModules *TableModules::self = nullptr;


wxString TypeModule::Name(E type)
{
    static const wxString names[Count] =
    {
        _("Контроллер"),
        _("Дисплей"),
        _("Клавиатура"),
        _("Модем")
    };

    return names[type];
}


TableModules::TableModules(wxWindow *parent) : wxListView(parent, wxID_ANY, wxDefaultPosition, { 500, 130 })
{
    self = this;

    AppendColumn(_("Модуль"), wxLIST_FORMAT_LEFT);
    AppendColumn(_("Версия"), wxLIST_FORMAT_LEFT, 130);
    AppendColumn(_("Файл"), wxLIST_FORMAT_LEFT, 130);
    AppendColumn(_("Статус"), wxLIST_FORMAT_LEFT, 130);
}


void TableModules::AppendModule(TypeModule::E type)
{
    if (std::find(lines.begin(), lines.end(), type) != lines.end())
    {
        return;
    }

    if (lines.size() == 0)
    {
        lines.push_back(type);
    }
    else
    {
        for (uint i = 0; i < lines.size(); i++)
        {
            if (lines[i] > type)
            {
                lines.insert(lines.begin() + i, type);   
                break;
            }
        }
    }

    if (std::find(lines.begin(), lines.end(), type) == lines.end())
    {
        lines.push_back(type);
    }

    for (uint i = 0; i < lines.size(); i++)
    {
        if (lines[i] == type)
        {
            InsertItem(i, "");

            wxListItem item;

            item.SetId(i);
            item.SetColumn(0);
            item.SetText(TypeModule::Name(type));

            SetItem(item);

            SetVersion(type, "-");
            SetFile(type, "-");
            SetStatus(type, "-");
        }
    }
}


void TableModules::SetVersion(TypeModule::E type, const wxString &version)
{
    long line = GetNumLine(type);

    if (line >= 0)
    {
        SetItem(line, 1, version);
    }
}


void TableModules::SetFile(TypeModule::E type, const wxString &file)
{
    long line = GetNumLine(type);

    if (line >= 0)
    {
        SetItem(line, 2, file);
    }
}


void TableModules::SetStatus(TypeModule::E type, const wxString &status)
{
    long line = GetNumLine(type);

    if (line >= 0)
    {
        SetItem(line, 3, status);
    }
}


void TableModules::TestView()
{
    AppendModule(TypeModule::Modem);
    AppendModule(TypeModule::Controller);
    AppendModule(TypeModule::Keyboard);
    AppendModule(TypeModule::Display);

    SetVersion(TypeModule::Controller, "alpro_v10");
    SetVersion(TypeModule::Display, "alpro_v10_lcd");
    SetVersion(TypeModule::Keyboard, "alpro_v10_button");

    SetFile(TypeModule::Controller, "controller.bin");
    SetFile(TypeModule::Display, "lcd.bin");
    SetFile(TypeModule::Keyboard, "button_v2.bin");

    SetStatus(TypeModule::Controller, _("Обновлено"));
    SetStatus(TypeModule::Display, _("Обновление 11%"));
    SetStatus(TypeModule::Keyboard, _("Можно обновить"));
}


long TableModules::GetNumLine(TypeModule::E type) const
{
    for (uint i = 0; i < lines.size(); i++)
    {
        if (lines[i] == type)
        {
            return (long)i;
        }
    }

    return -1;
}
