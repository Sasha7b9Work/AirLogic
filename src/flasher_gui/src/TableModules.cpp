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


TableModules::TableModules(wxWindow *parent) : wxListView(parent)
{
    self = this;

    AppendColumn(_("Модуль"), wxLIST_FORMAT_LEFT);
    AppendColumn(_("Версия"), wxLIST_FORMAT_LEFT);
    AppendColumn(_("Файл"), wxLIST_FORMAT_LEFT);
    AppendColumn(_("Статус"), wxLIST_FORMAT_LEFT);
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
        }
    }
}


void TableModules::SetVersion(TypeModule::E type, const wxString &version)
{

}


void TableModules::SetFile(TypeModule::E type, const wxString &file)
{

}


void TableModules::SetStatus(TypeModule::E type, const wxString &status)
{

}


void TableModules::TestView()
{
    TableModules::self->AppendModule(TypeModule::Modem);
    TableModules::self->AppendModule(TypeModule::Controller);
    TableModules::self->AppendModule(TypeModule::Display);
    TableModules::self->AppendModule(TypeModule::Keyboard);
}
