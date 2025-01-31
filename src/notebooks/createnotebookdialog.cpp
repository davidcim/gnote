/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <glibmm/i18n.h>

#include <gtkmm/table.h>
#include <gtkmm/stock.h>

#include "sharp/string.hpp"
#include "notebooks/createnotebookdialog.hpp"
#include "notebooks/notebookmanager.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "utils.hpp"

namespace gnote {
  namespace notebooks {

    CreateNotebookDialog::CreateNotebookDialog(Gtk::Window *parent, GtkDialogFlags f)
      : utils::HIGMessageDialog(parent, f, Gtk::MESSAGE_OTHER, Gtk::BUTTONS_NONE)
    {
      set_title(_("Create Notebook"));
      Gtk::Table *table = manage(new Gtk::Table (2, 2, false));
      table->set_col_spacings(6);
      
      Gtk::Label *label = manage(new Gtk::Label (_("N_otebook name:"), true));
      label->property_xalign() = 0;
      label->show ();
      
      m_nameEntry.signal_changed().connect(
        sigc::mem_fun(*this, &CreateNotebookDialog::on_name_entry_changed));
      m_nameEntry.set_activates_default(true);
      m_nameEntry.show ();
      label->set_mnemonic_widget(m_nameEntry);
      
      m_errorLabel.property_xalign() = 0;
      m_errorLabel.set_markup(
        Glib::ustring::compose("<span foreground='red' style='italic'>%1</span>",
            _("Name already taken")));
      
      table->attach (*label, 0, 1, 0, 1);
      table->attach (m_nameEntry, 1, 2, 0, 1);
      table->attach (m_errorLabel, 1, 2, 1, 2);
      table->show ();
      
      set_extra_widget(table);
      
      add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, false);
      add_button (IGnote::obj().icon_manager().get_icon(IconManager::NOTEBOOK_NEW, 16),
                  // Translation note: This is the Create button in the Create
                  // New Note Dialog.
                  _("C_reate"), Gtk::RESPONSE_OK, true);
      
      // Only let the Ok response be sensitive when
      // there's something in nameEntry
      set_response_sensitive (Gtk::RESPONSE_OK, false);
      m_errorLabel.hide ();

    }


    Glib::ustring CreateNotebookDialog::get_notebook_name()
    {
      return sharp::string_trim(m_nameEntry.get_text());
    }


    void CreateNotebookDialog::set_notebook_name(const Glib::ustring & value)
    {
      m_nameEntry.set_text(sharp::string_trim(value));
    }


    void CreateNotebookDialog::on_name_entry_changed()
    {
      bool nameTaken = false;
      if(NotebookManager::obj().notebook_exists(get_notebook_name())) {
        m_errorLabel.show ();
        nameTaken = true;
      } 
      else {
        m_errorLabel.hide ();
      }
      
      set_response_sensitive (Gtk::RESPONSE_OK,
        (get_notebook_name().empty() || nameTaken) ? false : true);

    }

  }
}
