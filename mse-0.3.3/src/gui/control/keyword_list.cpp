//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <gui/control/keyword_list.hpp>
#include <gui/icon_menu.hpp>
#include <data/set.hpp>
#include <data/game.hpp>
#include <data/keyword.hpp>
#include <data/action/value.hpp>
#include <data/action/keyword.hpp>
#include <data/format/clipboard.hpp>
#include <util/tagged_string.hpp>
#include <util/window_id.hpp>
#include <gfx/gfx.hpp>
#include <wx/clipbrd.h>

DECLARE_TYPEOF_COLLECTION(KeywordP);

// ----------------------------------------------------------------------------- : Events

DEFINE_EVENT_TYPE(EVENT_KEYWORD_SELECT);

// ----------------------------------------------------------------------------- : KeywordList

KeywordList::KeywordList(Window* parent, int id, long additional_style)
	: ItemList(parent, id, additional_style)
{
	// Add columns
	InsertColumn(0, _LABEL_("keyword"),  wxLIST_FORMAT_LEFT,    0);
	InsertColumn(1, _LABEL_("match"),    wxLIST_FORMAT_LEFT,  200);
	InsertColumn(2, _LABEL_("mode"),     wxLIST_FORMAT_LEFT,   60);
	InsertColumn(3, _LABEL_("uses"),     wxLIST_FORMAT_RIGHT,  50);
	InsertColumn(4, _LABEL_("reminder"), wxLIST_FORMAT_LEFT,  300);
}

KeywordList::~KeywordList() {
	storeColumns();
}
void KeywordList::storeColumns() {
	// TODO
}

void KeywordList::onBeforeChangeSet() {
	storeColumns();
}
void KeywordList::onChangeSet() {
	refreshList();
}

void KeywordList::onAction(const Action& action, bool undone) {
	TYPE_CASE(action, AddKeywordAction) {
		if (action.adding != undone) {
			// select the new keyword
			selectItem(action.keyword, false /*list will be refreshed anyway*/, true);
			refreshList();
		} else {
			long pos = selected_item_pos;
			refreshList();
			if (action.keyword == selected_item) {
				// select the next keyword, if not possible, select the last
				if (pos + 1 < GetItemCount()) {
					selectItemPos(pos, true);
				} else {
					selectItemPos(GetItemCount() - 1, true);
				}
			}
		}
	}
	TYPE_CASE(action, ValueAction) {
		KeywordTextValue* value = dynamic_cast<KeywordTextValue*>(action.valueP.get());
		if (value) {
			// this is indeed an action on a keyword, refresh
			refreshList();
		}
	}
	TYPE_CASE_(action, ChangeKeywordModeAction) {
		refreshList();
	}
}

// ----------------------------------------------------------------------------- : Clipboard

bool KeywordList::canCopy()  const { return !!selected_item; }
bool KeywordList::canCut()   const { return canCopy() && !getKeyword()->fixed; }
bool KeywordList::canPaste() const {
	return wxTheClipboard->IsSupported(KeywordDataObject::format);
}

bool KeywordList::doCopy() {
	if (!canCopy()) return false;
	if (!wxTheClipboard->Open()) return false;
	bool ok = wxTheClipboard->SetData(new KeywordDataObject(set, getKeyword())); // ignore result
	wxTheClipboard->Close();
	return ok;
}
bool KeywordList::doCut() {
	// cut = copy + delete
	if (!canCut()) return false;
	if (!doCopy()) return false;
	set->actions.add(new AddKeywordAction(REMOVE, *set, getKeyword()));
	return true;
}
bool KeywordList::doPaste() {
	// get data
	if (!canPaste()) return false;
	if (!wxTheClipboard->Open()) return false;
	KeywordDataObject data;
	bool ok = wxTheClipboard->GetData(data);
	wxTheClipboard->Close();
	if (!ok) return false;
	// add keyword to set
	KeywordP keyword = data.getKeyword(set);
	if (keyword) {
		set->actions.add(new AddKeywordAction(ADD, *set, keyword));
		return true;
	} else {
		return false;
	}
}

// ----------------------------------------------------------------------------- : KeywordListBase : for ItemList

String match_string(const Keyword& a) {
	return untag(replace_all(replace_all(
	            a.match,
		         _("<atom-param>"), LEFT_ANGLE_BRACKET),
		        _("</atom-param>"), RIGHT_ANGLE_BRACKET)
	       );
}

void KeywordList::getItems(vector<VoidP>& out) const {
	FOR_EACH(k, set->keywords) {
		k->fixed = false;
		out.push_back(k);
	}
	FOR_EACH(k, set->game->keywords) {
		k->fixed = true;
		out.push_back(k);
	}
}
void KeywordList::sendEvent() {
	KeywordSelectEvent ev(getKeyword());
	ProcessEvent(ev);
}
bool KeywordList::compareItems(void* a, void* b) const {
	const Keyword& ka = *(Keyword*)a;
	const Keyword& kb = *(Keyword*)b;
	switch(sort_by_column) {
		case 0:	return ka.keyword < kb.keyword;
		case 1: return ka.match   < kb.match;
		case 2: return ka.mode    < kb.mode;
		//case 3:
		case 4: return ka.reminder.getUnparsed() < kb.reminder.getUnparsed();
		default: // TODO: 3
				return ka.keyword < kb.keyword;
	}
}

// ----------------------------------------------------------------------------- : KeywordList : Item text

String KeywordList::OnGetItemText (long pos, long col) const {
	const Keyword& kw = *getKeyword(pos);
	switch(col) {
		case 0:		return kw.keyword;
		case 1:		return match_string(kw);
		case 2:		return kw.mode;
		case 3:		return _("?");
		case 4:		return kw.reminder.getUnparsed();
		default:	return wxEmptyString;
	}
}
int KeywordList::OnGetItemImage(long pos) const {
	return -1;
}

wxListItemAttr* KeywordList::OnGetItemAttr(long pos) const {
	// black for set keywords, grey for game keywords (uneditable)
	const Keyword& kw = *getKeyword(pos);
	if (!kw.fixed && kw.valid) return nullptr;
	if (!kw.valid) {
		item_attr.SetTextColour(*wxRED);
	} else if (kw.fixed) {
		item_attr.SetTextColour(lerp(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW),wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT),0.5));
	}
	return &item_attr;
}

// ----------------------------------------------------------------------------- : KeywordList : Context menu

void KeywordList::onContextMenu(wxContextMenuEvent&) {
	IconMenu m;
	m.Append(ID_EDIT_CUT,		_("cut"),			_CONTEXT_MENU_("cut"),				_HELP_("cut keyword"));
	m.Append(ID_EDIT_COPY,		_("copy"),			_CONTEXT_MENU_("copy"),				_HELP_("copy keyword"));
	m.Append(ID_EDIT_PASTE,		_("paste"),			_CONTEXT_MENU_("paste"),			_HELP_("paste keyword"));
	m.AppendSeparator();
	m.Append(ID_KEYWORD_ADD,	_("keyword_add"),	_CONTEXT_MENU_("add keyword"),		_HELP_("add keyword"));
	m.Append(ID_KEYWORD_REMOVE,	_("keyword_del"),	_CONTEXT_MENU_("remove keyword"),	_HELP_("remove keyword"));
	PopupMenu(&m);
}

BEGIN_EVENT_TABLE(KeywordList, ItemList)
	EVT_CONTEXT_MENU(KeywordList::onContextMenu)
END_EVENT_TABLE  ()