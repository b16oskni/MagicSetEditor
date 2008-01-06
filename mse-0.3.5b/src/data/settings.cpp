//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <data/settings.hpp>
#include <data/installer.hpp>
#include <data/game.hpp>
#include <data/stylesheet.hpp>
#include <data/field.hpp>
#include <data/export_template.hpp>
#include <data/word_list.hpp>
#include <util/reflect.hpp>
#include <util/platform.hpp>
#include <util/io/reader.hpp>
#include <util/io/writer.hpp>
#include <util/delayed_index_maps.hpp>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>

DECLARE_TYPEOF_COLLECTION(AutoReplaceP);

// ----------------------------------------------------------------------------- : Extra types

IMPLEMENT_REFLECTION_ENUM(CheckUpdates) {
	VALUE_N("if connected", CHECK_IF_CONNECTED); //default
	VALUE_N("always",       CHECK_ALWAYS);
	VALUE_N("never",        CHECK_NEVER);
}

IMPLEMENT_REFLECTION_ENUM(InstallType) {
	VALUE_N("default",	INSTALL_DEFAULT); //default
	VALUE_N("local",	INSTALL_LOCAL);
	VALUE_N("global",	INSTALL_GLOBAL);
}

IMPLEMENT_REFLECTION_ENUM(FilenameConflicts) {
	VALUE_N("keep old",			CONFLICT_KEEP_OLD);
	VALUE_N("overwrite",		CONFLICT_OVERWRITE);
	VALUE_N("number",			CONFLICT_NUMBER);
	VALUE_N("number overwrite",	CONFLICT_NUMBER_OVERWRITE);
}

const int COLUMN_NOT_INITIALIZED = -100000;

ColumnSettings::ColumnSettings()
	: width(100), position(COLUMN_NOT_INITIALIZED), visible(false)
{}

// dummy for ColumnSettings reflection
ScriptValueP to_script(const ColumnSettings&) { return script_nil; }

IMPLEMENT_REFLECTION_NO_SCRIPT(ColumnSettings) {
	REFLECT(width);
	REFLECT(position);
	REFLECT(visible);
}

GameSettings::GameSettings()
	: sort_cards_ascending(true)
	, images_export_filename(_("{card.name}.jpg"))
	, images_export_conflicts(CONFLICT_NUMBER_OVERWRITE)
	, use_auto_replace(true)
	, initialized(false)
{}

void GameSettings::initDefaults(const Game& game) {
	if (initialized) return;
	initialized = true;
	// init auto_replaces, copy from game file
	FOR_EACH_CONST(ar, game.auto_replaces) {
		// do we have this one?
		bool already_have = false;
		FOR_EACH(ar2, auto_replaces) {
			if (ar->match == ar2->match) {
				ar2->custom = false;
				already_have = true;
				break;
			}
		}
		if (!already_have) {
			// TODO: when we start saving games, clone here
			ar->custom = false;
			auto_replaces.push_back(ar);
		}
	}
}

IMPLEMENT_REFLECTION_NO_SCRIPT(GameSettings) {
	REFLECT(default_stylesheet);
	REFLECT(default_export);
	REFLECT_N("cardlist columns",     columns);
	REFLECT(sort_cards_by);
	REFLECT(sort_cards_ascending);
	REFLECT(images_export_filename);
	REFLECT(images_export_conflicts);
	REFLECT(use_auto_replace);
	REFLECT(auto_replaces);
}


StyleSheetSettings::StyleSheetSettings()
	: card_zoom         (1.0,  true)
	, card_angle        (0,    true)
	, card_anti_alias   (true, true)
	, card_borders      (true, true)
	, card_draw_editing (true, true)
	, card_normal_export(true, true)
{}

void StyleSheetSettings::useDefault(const StyleSheetSettings& ss) {
	if (card_zoom         .isDefault()) card_zoom         .assignDefault(ss.card_zoom);
	if (card_angle        .isDefault()) card_angle        .assignDefault(ss.card_angle);
	if (card_anti_alias   .isDefault()) card_anti_alias   .assignDefault(ss.card_anti_alias);
	if (card_borders      .isDefault()) card_borders      .assignDefault(ss.card_borders);
	if (card_draw_editing .isDefault()) card_draw_editing .assignDefault(ss.card_draw_editing);
	if (card_normal_export.isDefault()) card_normal_export.assignDefault(ss.card_normal_export);
}

IMPLEMENT_REFLECTION_NO_SCRIPT(StyleSheetSettings) {
	REFLECT(card_zoom);
	REFLECT(card_angle);
	REFLECT(card_anti_alias);
	REFLECT(card_borders);
	REFLECT(card_draw_editing);
	REFLECT(card_normal_export);
}

// ----------------------------------------------------------------------------- : Settings

Settings settings;

Settings::Settings()
	: locale               (_("en"))
	, set_window_maximized (false)
	, set_window_width     (790)
	, set_window_height    (300)
	, card_notes_height    (40)
	, symbol_grid_size     (30)
	, symbol_grid          (true)
	, symbol_grid_snap     (false)
	, updates_url          (_("http://magicseteditor.sourceforge.net/updates"))
	, check_updates        (CHECK_IF_CONNECTED)
	, install_type         (INSTALL_DEFAULT)
	, website_url          (_("http://magicseteditor.sourceforge.net/"))
{}

void Settings::addRecentFile(const String& filename) {
	// get absolute path
	wxFileName fn(filename);
	fn.Normalize();
	String filenameAbs = fn.GetFullPath();
	// remove duplicates
	recent_sets.erase(
		remove(recent_sets.begin(), recent_sets.end(), filenameAbs),
		recent_sets.end()
	);
	// add to front of list
	recent_sets.insert(recent_sets.begin(), filenameAbs);
	// enforce size limit
	if (recent_sets.size() > max_recent_sets) recent_sets.resize(max_recent_sets);
}

GameSettings& Settings::gameSettingsFor(const Game& game) {
	GameSettingsP& gs = game_settings[game.name()];
	if (!gs) gs = new_intrusive<GameSettings>();
	gs->initDefaults(game);
	return *gs;
}
ColumnSettings& Settings::columnSettingsFor(const Game& game, const Field& field) {
	// Get game info
	GameSettings& gs = gameSettingsFor(game);
	// Get column info
	ColumnSettings& cs = gs.columns[field.name];
	if (cs.position == COLUMN_NOT_INITIALIZED) {
		// column info not set, initialize based on the game
		cs.visible  = field.card_list_visible;
		cs.position = field.card_list_column;
		cs.width    = field.card_list_width;
	}
	return cs;
}
StyleSheetSettings& Settings::stylesheetSettingsFor(const StyleSheet& stylesheet) {
	StyleSheetSettingsP& ss = stylesheet_settings[stylesheet.name()];
	if (!ss) ss = new_intrusive<StyleSheetSettings>();
	ss->useDefault(default_stylesheet_settings); // update default settings
	return *ss;
}

IndexMap<FieldP,ValueP>& Settings::exportOptionsFor(const ExportTemplate& export_template) {
	return export_options.get(export_template.name(), export_template.option_fields);
}

/// Retrieve the directory to use for settings and other data files
String user_settings_dir() {
	String dir = wxStandardPaths::Get().GetUserDataDir();
	if (!wxDirExists(dir)) wxMkdir(dir);
	return dir + _("/");
}

String Settings::settingsFile() {
//	return user_settings_dir() + _("mse.config");
	return user_settings_dir() + _("mse8.config"); // use different file during development of C++ port
}

IMPLEMENT_REFLECTION_NO_SCRIPT(Settings) {
	REFLECT_ALIAS(300,         "style settings",         "stylesheet settings");
	REFLECT_ALIAS(300, "default style settings", "default stylesheet settings");
	REFLECT(locale);
	REFLECT(recent_sets);
	REFLECT(set_window_maximized);
	REFLECT(set_window_width);
	REFLECT(set_window_height);
	REFLECT(card_notes_height);
	REFLECT(symbol_grid_size);
	REFLECT(symbol_grid);
	REFLECT(symbol_grid_snap);
	REFLECT(default_game);
	REFLECT(apprentice_location);
	REFLECT(updates_url);
	REFLECT(check_updates);
	REFLECT(install_type);
	REFLECT(website_url);
	REFLECT(game_settings);
	REFLECT(stylesheet_settings);
	REFLECT(default_stylesheet_settings);
	REFLECT(export_options);
}

void Settings::clear() {
	recent_sets.clear();
	game_settings.clear();
	stylesheet_settings.clear();
	default_stylesheet_settings = StyleSheetSettings();
	export_options.clear();
}

void Settings::read() {
	// clear current settings, otherwise we duplicate vector elements
	clear();
	// (re)load settings
	String filename = settingsFile();
	if (wxFileExists(filename)) {
		// settings file not existing is not an error
		shared_ptr<wxFileInputStream> file = new_shared1<wxFileInputStream>(filename);
		if (!file->Ok()) return; // failure is not an error
		Reader reader(file, filename);
		reader.handle_greedy(*this);
	}
}

void Settings::write() {
	Writer writer(new_shared1<wxFileOutputStream>(settingsFile()));
	writer.handle(*this);
}