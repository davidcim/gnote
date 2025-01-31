/*
 * gnote
 *
 * Copyright (C) 2011-2014,2016-2017,2019 Aurimas Cernius
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

#include "config.h"

#include "debug.hpp"
#include "ignote.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "remotecontrolproxy.hpp"
#include "search.hpp"
#include "tag.hpp"
#include "itagmanager.hpp"
#include "dbus/remotecontrol.hpp"
#include "sharp/map.hpp"

namespace gnote {


  RemoteControl::RemoteControl(const Glib::RefPtr<Gio::DBus::Connection> & cnx, NoteManager& manager,
                               const char * path, const char * interface_name,
                               const Glib::RefPtr<Gio::DBus::InterfaceInfo> & gnote_interface)
    : IRemoteControl(cnx, path, interface_name, gnote_interface)
    , m_manager(manager)
  {
    DBG_OUT("initialized remote control");
    m_manager.signal_note_added.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_added));
    m_manager.signal_note_deleted.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_deleted));
    m_manager.signal_note_saved.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_saved));
  }


  RemoteControl::~RemoteControl()
  {
  }

  bool RemoteControl::AddTagToNote(const Glib::ustring& uri, const Glib::ustring& tag_name)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }
    Tag::Ptr tag = ITagManager::obj().get_or_create_tag(tag_name);
    note->add_tag (tag);
    return true;
  }


  Glib::ustring RemoteControl::CreateNamedNote(const Glib::ustring& linked_title)
  {
    NoteBase::Ptr note = m_manager.find(linked_title);
    if (note)
      return "";

    try {
      note = m_manager.create (linked_title);
      return note->uri();
    } 
    catch (const std::exception & e) {
      ERR_OUT(_("Exception thrown when creating note: %s"), e.what());
    }
    return "";
  }

  Glib::ustring RemoteControl::CreateNote()
  {
    try {
      NoteBase::Ptr note = m_manager.create ();
      return note->uri();
    } 
    catch(...)
    {
    }
    return "";
  }

  bool RemoteControl::DeleteNote(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }

    m_manager.delete_note (note);
    return true;

  }

  bool RemoteControl::DisplayNote(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }

    present_note(note);
    return true;
  }


  bool RemoteControl::DisplayNoteWithSearch(const Glib::ustring& uri, const Glib::ustring& search)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }

    MainWindow & window(present_note(note));
    window.set_search_text(search);
    window.show_search_bar();

    return true;
  }


  void RemoteControl::DisplaySearch()
  {
    IGnote::obj().open_search_all().present();
  }


  void RemoteControl::DisplaySearchWithText(const Glib::ustring& search_text)
  {
    MainWindow & recent_changes = IGnote::obj().get_main_window();
    recent_changes.set_search_text(search_text);
    recent_changes.present();
    recent_changes.show_search_bar();
  }


  Glib::ustring RemoteControl::FindNote(const Glib::ustring& linked_title)
  {
    NoteBase::Ptr note = m_manager.find(linked_title);
    return (!note) ? "" : note->uri();
  }


  Glib::ustring RemoteControl::FindStartHereNote()
  {
    NoteBase::Ptr note = m_manager.find_by_uri(m_manager.start_note_uri());
    return (!note) ? "" : note->uri();
  }


  std::vector<Glib::ustring> RemoteControl::GetAllNotesWithTag(const Glib::ustring& tag_name)
  {
    Tag::Ptr tag = ITagManager::obj().get_tag(tag_name);
    if (!tag)
      return std::vector<Glib::ustring>();

    std::vector<Glib::ustring> tagged_note_uris;
    auto notes = tag->get_notes();
    for(NoteBase *iter : notes) {
      tagged_note_uris.push_back(iter->uri());
    }
    return tagged_note_uris;
  }


  int32_t RemoteControl::GetNoteChangeDate(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return -1;
    return note->metadata_change_date().sec();
  }


  Glib::ustring RemoteControl::GetNoteCompleteXml(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return note->get_complete_note_xml();

  }


  Glib::ustring RemoteControl::GetNoteContents(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return std::static_pointer_cast<Note>(note)->text_content();
  }


  Glib::ustring RemoteControl::GetNoteContentsXml(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return note->xml_content();
  }


  int32_t RemoteControl::GetNoteCreateDate(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return -1;
    return note->create_date().sec();
  }


  Glib::ustring RemoteControl::GetNoteTitle(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return note->get_title();
  }


  std::vector<Glib::ustring> RemoteControl::GetTagsForNote(const Glib::ustring& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return std::vector<Glib::ustring>();

    std::vector<Glib::ustring> tags;
    std::vector<Tag::Ptr> l = note->get_tags();
    for(auto & tag : l) {
      tags.push_back(tag->normalized_name());
    }
    return tags;
  }


bool RemoteControl::HideNote(const Glib::ustring& uri)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if (!note)
    return false;

  NoteWindow *window = std::static_pointer_cast<Note>(note)->get_window();
  if(window == NULL) {
    return true;
  }
  MainWindow *win = MainWindow::get_owning(*window);
  if(win) {
    win->unembed_widget(*window);
  }
  return true;
}


std::vector<Glib::ustring> RemoteControl::ListAllNotes()
{
  std::vector<Glib::ustring> uris;
  for(const NoteBase::Ptr & iter : m_manager.get_notes()) {
    uris.push_back(iter->uri());
  }
  return uris;
}


bool RemoteControl::NoteExists(const Glib::ustring& uri)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  return note != NULL;
}


bool RemoteControl::RemoveTagFromNote(const Glib::ustring& uri, 
                                      const Glib::ustring& tag_name)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if (!note)
    return false;
  Tag::Ptr tag = ITagManager::obj().get_tag(tag_name);
  if (tag) {
    note->remove_tag (tag);
  }
  return true;
}


std::vector<Glib::ustring> RemoteControl::SearchNotes(const Glib::ustring& query,
                                                      const bool& case_sensitive)
{
  if (query.empty())
    return std::vector<Glib::ustring>();

  Search search(m_manager);
  std::vector<Glib::ustring> list;
  Search::ResultsPtr results =
    search.search_notes(query, case_sensitive, notebooks::Notebook::Ptr());

  for(Search::Results::const_reverse_iterator iter = results->rbegin();
      iter != results->rend(); iter++) {

    list.push_back(iter->second->uri());
  }

  return list;
}


bool RemoteControl::SetNoteCompleteXml(const Glib::ustring& uri, 
                                       const Glib::ustring& xml_contents)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }
    
  note->load_foreign_note_xml(xml_contents, CONTENT_CHANGED);
  return true;
}


bool RemoteControl::SetNoteContents(const Glib::ustring& uri, 
                                    const Glib::ustring& text_contents)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }

  std::static_pointer_cast<Note>(note)->set_text_content(text_contents);
  return true;
}


bool RemoteControl::SetNoteContentsXml(const Glib::ustring& uri, 
                                       const Glib::ustring& xml_contents)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }
  note->set_xml_content(xml_contents);
  return true;
}


Glib::ustring RemoteControl::Version()
{
  return VERSION;
}



void RemoteControl::on_note_added(const NoteBase::Ptr & note)
{
  if(note) {
    NoteAdded(note->uri());
  }
}


void RemoteControl::on_note_deleted(const NoteBase::Ptr & note)
{
  if(note) {
    NoteDeleted(note->uri(), note->get_title());
  }
}


void RemoteControl::on_note_saved(const NoteBase::Ptr & note)
{
  if(note) {
    NoteSaved(note->uri());
  }
}


MainWindow & RemoteControl::present_note(const NoteBase::Ptr & note)
{
  return *MainWindow::present_default(std::static_pointer_cast<Note>(note));
}


}
