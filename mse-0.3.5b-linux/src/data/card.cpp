//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <data/card.hpp>
#include <data/game.hpp>
#include <data/stylesheet.hpp>
#include <data/field.hpp>
#include <util/error.hpp>
#include <util/reflect.hpp>
#include <util/delayed_index_maps.hpp>

DECLARE_TYPEOF_COLLECTION(FieldP);
DECLARE_TYPEOF_NO_REV(IndexMap<FieldP COMMA ValueP>);

// ----------------------------------------------------------------------------- : Card

Card::Card()
	: has_styling(false)
{
	if (!game_for_reading()) {
		throw InternalError(_("game_for_reading not set"));
	}
	data.init(game_for_reading()->card_fields);
}

Card::Card(const Game& game)
	: has_styling(false)
{
	data.init(game.card_fields);
}

String Card::identification() const {
	// an identifying field
	FOR_EACH_CONST(v, data) {
		if (v->fieldP->identifying) {
			return v->toString();
		}
	}
	// otherwise the first field
	if (!data.empty()) {
		return data.at(0)->toString();
	} else {
		return wxEmptyString;
	}
}

IndexMap<FieldP, ValueP>& Card::extraDataFor(const StyleSheet& stylesheet) {
	return extra_data.get(stylesheet.name(), stylesheet.extra_card_fields);
}

void mark_dependency_member(const Card& card, const String& name, const Dependency& dep) {
	mark_dependency_member(card.data, name, dep);
}

IMPLEMENT_REFLECTION(Card) {
	REFLECT(stylesheet);
	REFLECT(has_styling);
	if (has_styling) {
		if (stylesheet) {
			REFLECT_IF_READING styling_data.init(stylesheet->styling_fields);
			REFLECT(styling_data);
		} else if (stylesheet_for_reading()) {
			REFLECT_IF_READING styling_data.init(stylesheet_for_reading()->styling_fields);
			REFLECT(styling_data);
		} else if (tag.reading()) {
			has_styling = false; // We don't know the style, this can be because of copy/pasting
		}
	}
	REFLECT(notes);
	REFLECT_NO_SCRIPT(extra_data); // don't allow scripts to depend on style specific data
	REFLECT_NAMELESS(data);
}