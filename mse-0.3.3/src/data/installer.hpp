//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#ifndef HEADER_DATA_INSTALLER
#define HEADER_DATA_INSTALLER

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/io/package.hpp>

// ----------------------------------------------------------------------------- : Installer

/// A package that contains other packages that can be installed
class Installer : public Packaged {
  public:
	String prefered_filename;	///< What filename should be used (by default)
	vector<String> packages;	///< Packages to install
	
	/// Install all the packages
	void install();
	
	/// Add a package to the installer (if it is not already added).
	/** The first package gives the name of the installer.
	 */
	void addPackage(const Packaged& package);
	
  protected:
	String typeName() const;
	DECLARE_REFLECTION();
};

// ----------------------------------------------------------------------------- : EOF
#endif