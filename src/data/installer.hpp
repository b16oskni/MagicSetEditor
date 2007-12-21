//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#ifndef HEADER_DATA_INSTALLER
#define HEADER_DATA_INSTALLER

// ----------------------------------------------------------------------------- : Includes

#include <data/settings.hpp>
#include <util/prec.hpp>
#include <util/io/package.hpp>

// ----------------------------------------------------------------------------- : InstallType
// Platform default install directory.
#ifdef __WXMSW__
#define DEFAULT_INSTALL_LOCAL false
#else
#define DEFAULT_INSTALL_LOCAL true
#endif

inline bool isInstallLocal(const InstallType& type)
{
	return type == INSTALL_DEFAULT ? DEFAULT_INSTALL_LOCAL : type == INSTALL_LOCAL;
}

// ----------------------------------------------------------------------------- : Installer

/// A package that contains other packages that can be installed
class Installer : public Packaged {
  public:
	String prefered_filename;	///< What filename should be used (by default)
	vector<String> packages;	///< Packages to install
	
	/// Load an installer from a file, and run it
	static void installFrom(const String& filename, bool message_on_success, bool local);
	/// Install all the packages
	void install(bool local, bool check_dependencies = true);
	/// Install a specific package
	void install(const String& package);
	
	/// Add a package to the installer (if it is not already added).
	/** If the package is named *.mse-installer uses it as the filename instead */
	void addPackage(const String& package);
	/// Add a package to the installer (if it is not already added).
	/** The first package gives the name of the installer.
	 */
	void addPackage(Packaged& package);
	
  protected:
	String typeName() const;
	DECLARE_REFLECTION();
};

// ----------------------------------------------------------------------------- : EOF
#endif