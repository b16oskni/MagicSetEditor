//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <render/card/viewer.hpp>
#include <render/value/viewer.hpp>
#include <data/set.hpp>
#include <data/stylesheet.hpp>
#include <data/card.hpp>
#include <data/field.hpp>
#include <data/settings.hpp>
#include <data/action/value.hpp>
#include <data/action/set.hpp>
#include <gui/util.hpp> // clearDC

DECLARE_TYPEOF_COLLECTION(ValueViewerP);
DECLARE_TYPEOF_NO_REV(IndexMap<FieldP COMMA StyleP>);

// ----------------------------------------------------------------------------- : DataViewer

DataViewer::DataViewer() : drawing(false) {}
DataViewer::~DataViewer() {}

// ----------------------------------------------------------------------------- : Drawing

void DataViewer::draw(DC& dc) {
	StyleSheetSettings& ss = settings.stylesheetSettingsFor(*stylesheet);
	RotatedDC rdc(dc, ss.card_angle(), stylesheet->getCardRect(), ss.card_zoom(),
	              nativeLook() ? QUALITY_LOW : (ss.card_anti_alias() ? QUALITY_AA : QUALITY_SUB_PIXEL),
	              true);
	draw(rdc, stylesheet->card_background);
}
void DataViewer::draw(RotatedDC& dc, const Color& background) {
	if (!set) return; // no set specified, don't draw anything
	drawing = true;
	// fill with background color
	clearDC(dc.getDC(), background);
	// update style scripts
	try {
		Context& ctx = getContext();
		FOR_EACH(v, viewers) {
			if (v->getStyle()->update(ctx)) {
				v->getStyle()->tellListeners();
			}
		}
	} catch (const Error& e) {
		handle_error(e, false, false);
	}
	// draw values
	FOR_EACH(v, viewers) { // draw low z index fields first
		if (v->getStyle()->visible) {// visible
			try {
				drawViewer(dc, *v);
			} catch (const Error& e) {
				handle_error(e, false, false);
			}
		}
	}
	drawing = false;
}
void DataViewer::drawViewer(RotatedDC& dc, ValueViewer& v) {
	v.draw(dc);
}

// ----------------------------------------------------------------------------- : Utility for ValueViewers

bool   DataViewer::nativeLook()    const { return false; }
bool   DataViewer::drawBorders()   const { return false; }
bool   DataViewer::drawEditing()   const { return false; }
bool   DataViewer::drawFocus()     const { return false; }
wxPen  DataViewer::borderPen(bool) const { return wxPen(); }
ValueViewer* DataViewer::focusedViewer() const { return nullptr; }
Context& DataViewer::getContext()  const { return set->getContext(card); }

Rotation DataViewer::getRotation() const {
	if (!stylesheet) stylesheet = set->stylesheet;
	StyleSheetSettings& ss = settings.stylesheetSettingsFor(*stylesheet);
	return Rotation(ss.card_angle(), stylesheet->getCardRect(), ss.card_zoom(), true);
}

// ----------------------------------------------------------------------------- : Setting data

void DataViewer::setCard(const CardP& card, bool refresh) {
	if (!card) return; // TODO: clear viewer?
	StyleSheetP new_stylesheet = set->stylesheetForP(card);
	if (!refresh && this->card == card && this->stylesheet == new_stylesheet) return; // already set
	assert(set);
	this->card = card;
	stylesheet = new_stylesheet;
	setStyles(stylesheet, stylesheet->card_style);
	setData(card->data);
	onChangeSize();
}

void DataViewer::onChangeSet() {
	viewers.clear();
	onInit();
	onChange();
	onChangeSize();
}

// ----------------------------------------------------------------------------- : Viewers

struct CompareViewer {
	bool operator() (const ValueViewerP& a, const ValueViewerP& b) {
		return a->getStyle()->z_index < b->getStyle()->z_index;
	}
};

void DataViewer::setStyles(const StyleSheetP& stylesheet, IndexMap<FieldP,StyleP>& styles) {
	if (!viewers.empty() && styles.contains(viewers.front()->getStyle())) {
		// already using these styles
		return;
	}
	this->stylesheet = stylesheet;
	// create viewers
	viewers.clear();
	FOR_EACH(s, styles) {
		if ((s->visible || s->visible.isScripted()) &&
		    nativeLook() || (
		      (s->width   || s->width  .isScripted()) &&
		      (s->height  || s->height .isScripted()))) {
			// no need to make a viewer for things that are always invisible
			ValueViewerP viewer = makeViewer(s);
			if (viewer) viewers.push_back(viewer);
		}
	}
	// sort viewers by z-index of style
	stable_sort(viewers.begin(), viewers.end(), CompareViewer());
	onInit();
}

void DataViewer::setData(IndexMap<FieldP,ValueP>& values) {
	FOR_EACH(v, viewers) {
		v->setValue(values[v->getField()]);
	}
	onChange();
}


ValueViewerP DataViewer::makeViewer(const StyleP& style) {
	return style->makeViewer(*this, style);
}

void DataViewer::onAction(const Action& action, bool undone) {
	TYPE_CASE_(action, DisplayChangeAction) {
		// refresh
		setCard(card, true);
		return;
	}
	TYPE_CASE(action, ValueAction) {
		FOR_EACH(v, viewers) {
			if (v->getValue()->equals( action.valueP.get() )) {
				// refresh the viewer
				v->onAction(action, undone);
				onChange();
				return;
			}
		}
	}
	TYPE_CASE(action, ScriptValueEvent) {
		if (action.card == card.get()) {
			FOR_EACH(v, viewers) {
				if (v->getValue().get() == action.value) {
					// refresh the viewer
					v->onAction(action, undone);
					onChange();
					return;
				}
			}
		}
	}
/*//%	TYPE_CASE(action, ScriptStyleEvent) {
		if (action.stylesheet == stylesheet.get()) {
			FOR_EACH(v, viewers) {
				if (v->getStyle().get() == action.style) {
					// refresh the viewer
					v->onStyleChange();
					if (!drawing) onChange();
					return;
				}
			}
		}
	}*/
}