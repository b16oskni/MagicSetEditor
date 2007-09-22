//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2006 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <gui/util.hpp>
#include <util/error.hpp>
#include <util/rotation.hpp>
#include <wx/mstream.h>
#include <wx/renderer.h>

#if wxUSE_UXTHEME
	#include <wx/msw/uxtheme.h>
	#include <tmschema.h>
	#include <shlobj.h>
#endif

// ----------------------------------------------------------------------------- : Window related

// Id of the control that has the focus, or -1 if no control has the focus
int focused_control(const Window* window) {
	Window* focused_window = wxWindow::FindFocus();
	// is this window actually inside this panel?
	if (focused_window && wxWindow::FindWindowById(focused_window->GetId(), window) == focused_window) {
		return focused_window->GetId();
	} else {
		return -1; // no window has the focus, or it has a different parent/ancestor
	}
}

// ----------------------------------------------------------------------------- : DC related

/// Fill a DC with a single color
void clearDC(DC& dc, const wxBrush& brush) {
	wxSize size = dc.GetSize();
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(brush);
	dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
}

void clearDC_black(DC& dc) {
	#if !BITMAPS_DEFAULT_BLACK
		// On windows 9x it seems that bitmaps are not black by default
		clearDC(dc, *wxBLACK_BRUSH);
	#endif
}

void draw_checker(RotatedDC& dc, const RealRect& rect) {
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawRectangle(rect);
	dc.SetBrush(Color(235,235,235));
	const double checker_size = 10;
	int odd = 0;
	for (double y = 0 ; y < rect.height ; y += checker_size) {
		for (double x = odd * checker_size ; x < rect.width ; x += checker_size * 2) {
			dc.DrawRectangle(RealRect(
								rect.x + x,
								rect.y + y,
								min(checker_size, rect.width  - x),
								min(checker_size, rect.height - y)
							));
		}
		odd = 1 - odd;
	}
}

// ----------------------------------------------------------------------------- : Image related

Image load_resource_image(const String& name) {
	#ifdef __WXMSW__
		// Load resource
		// based on wxLoadUserResource
		// The image can be in an IMAGE resource, in any file format
		HRSRC hResource = ::FindResource(wxGetInstance(), name, _("IMAGE"));
		if ( hResource == 0 ) throw InternalError(String::Format(_("Resource not found: %s"), name));
		
		HGLOBAL hData = ::LoadResource(wxGetInstance(), hResource);
		if ( hData == 0 ) throw InternalError(String::Format(_("Resource not an image: %s"), name));
		
		char* data = (char *)::LockResource(hData);
		if ( !data ) throw InternalError(String::Format(_("Resource cannot be locked: %s"), name));

		int len = ::SizeofResource(wxGetInstance(), hResource);
		wxMemoryInputStream stream(data, len);
		return wxImage(stream);
	#endif
}

// ----------------------------------------------------------------------------- : Platform look

// Draw a basic 3D border
void draw3DBorder(DC& dc, int x1, int y1, int x2, int y2) {
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW));
	dc.DrawLine(x1, y1, x2, y1);
	dc.DrawLine(x1, y1, x1, y2);
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));
	dc.DrawLine(x1-1, y1-1, x2+1, y1-1);
	dc.DrawLine(x1-1, y1-1, x1-1, y2+1);
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT));
	dc.DrawLine(x1, y2, x2, y2);
	dc.DrawLine(x2, y1, x2, y2+1);
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT));
	dc.DrawLine(x1-1, y2+1, x2+1, y2+1);
	dc.DrawLine(x2+1, y1-1, x2+1, y2+2);
}

void draw_control_border(Window* win, DC& dc, const wxRect& rect) {
	#if wxUSE_UXTHEME
		RECT r;
		wxUxThemeEngine *themeEngine = wxUxThemeEngine::Get();
		if (themeEngine && themeEngine->IsAppThemed()) {
			wxUxThemeHandle hTheme(win, _("EDIT"));
			r.left = rect.x -1;
			r.top = rect.y  -1;
			r.right = rect.x + rect.width + 1;
			r.bottom = rect.y + rect.height + 1;
			if (hTheme) {
				wxUxThemeEngine::Get()->DrawThemeBackground(
					(HTHEME)hTheme,
					(HDC)dc.GetHDC(),
					EP_EDITTEXT,
					ETS_NORMAL,
					&r,
					NULL
				);
				return;
			}
		}
		r.left   = rect.x - 2;
		r.top    = rect.y - 2;
		r.right  = rect.x + rect.width  + 2;
		r.bottom = rect.y + rect.height + 2;
		DrawEdge((HDC)dc.GetHDC(), &r, EDGE_SUNKEN, BF_RECT);
	#else
		draw3DBorder(dc, rect.x - 1, rect.y - 1, rect.x + rect.width, rect.y + rect.height);
	#endif
}

// portable, based on wxRendererGeneric::DrawComboBoxDropButton
void draw_menu_arrow(Window* win, DC& dc, const wxRect& rect, bool active) {
	wxPoint pt[] =
		{	wxPoint(0, 0)
		,	wxPoint(4, 4)
		,	wxPoint(0, 8)
		};
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxSystemSettings::GetColour(active ? wxSYS_COLOUR_HIGHLIGHTTEXT : wxSYS_COLOUR_WINDOWTEXT));
	dc.DrawPolygon(3, pt, rect.x + rect.width - 6, rect.y + (rect.height - 9) / 2);
}

void draw_drop_down_arrow(Window* win, DC& dc, const wxRect& rect, bool active) {
	wxRendererNative& rn = wxRendererNative::GetDefault();
	int w = wxSystemSettings::GetMetric(wxSYS_VSCROLL_ARROW_X); // drop down arrow is same size
	rn.DrawComboBoxDropButton(win, dc, 
		wxRect(rect.x + rect.width - w, rect.y, w, rect.height)
		, active ? wxCONTROL_PRESSED : 0);
}