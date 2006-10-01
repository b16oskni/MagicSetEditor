//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2006 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#ifndef HEADER_GUI_SYMBOL_VIEWER
#define HEADER_GUI_SYMBOL_VIEWER

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/rotation.hpp>
#include <data/symbol.hpp>
#include <gfx/bezier.hpp>

// ----------------------------------------------------------------------------- : Symbol Viewer

enum HighlightStyle {
	HIGHLIGHT_BORDER,
	HIGHLIGHT_INTERIOR
};

/// Class that knows how to draw a symbol
class SymbolViewer : public SymbolView {
  public:
	// --------------------------------------------------- : Data
	SymbolViewer(const SymbolP& symbol, double borderRadius = 0.05);
	
	// drawing
	double borderRadius;

	// --------------------------------------------------- : Point translation
	
	Rotation rotation; //^ Object that handles rotation, scaling and translation
	
	// --------------------------------------------------- : Drawing

  public:
	/// Draw the symbol to a dc
	void draw(DC& dc);
		
	void highlightPart(DC& dc, const SymbolPart& part, HighlightStyle style);
		
  private:
	/// Combines a symbol part with what is currently drawn, the border and interior are drawn separatly
	/** directB/directI are true if the border/interior is the screen dc, false if it
	 *  is a temporary 1 bit one
	 */
	void combineSymbolPart(const SymbolPart& part, DC& border, DC& interior, bool directB, bool directI);
	
	/// Draw a symbol part, draws the border and the interior to separate DCs
	/** The DCs may be null. directB should be true when drawing the border directly to the screen.
	 *  The **Col parameters give the color to use for the (interior of) the border and the interior
	 *  default should be white (255) border and black (0) interior.
	 */
	void drawSymbolPart(const SymbolPart& part, DC* border, DC* interior, int borderCol, int interiorCol, bool directB);
/*	
	// ------------------- Bezier curve calculation
	
	// Calculate the points on a bezier curve between p0 and p1
	// Stores the Points in p_out, at most count points are stored
	// after this call p_out points to just beyond the last point
	void calcBezierPoint(const ControlPointP& p0, const ControlPointP& p1, wxPoint*& p_out, UInt count);
	
	// Subdivide a bezier curve by adding at most count points
	//   p0 = c(t0), p1 = c(p1)
	// subdivides linearly between t0 and t1, and only when necessary
	// adds points to p_out and increments the pointer when a point is added
	void calcBezierOpt(const BezierCurve& c, const Vector2D& p0, const Vector2D& p1, double t0, double t1, wxPoint*& p_out, UInt count);
*/};

// ----------------------------------------------------------------------------- : EOF
#endif
