This directory contains the necessary configuration files to create the API documentation for the Lumberyard Engine.

The documentation for the doxygen tool can be found at:

	http://www.stack.nl/~dimitri/doxygen/

The main configuration of Doxygen is found in the file:

	Doxyfile

The layout of the major components in the documentation is controlled by the file:

	layout.xml

Consult the doxygen documentation to understand how to modify the configuration and the layout.

Additionally, there in the directory doc-inputs you will find external documentation files with the suffix ".dox" and ".md".

.dox files are in doxygen comment format and .md files are in markdown format (See doxygen documentation for further information).

The three key files here are:

	groups.dox:		The module structure roughly matching the original CryEngine documentation
	auto-groups.dox		Additional module structure induced from the directory structure of the source.
	index.md:		A front page for the documentation.

There are also some key tools and files in the main directory:

	doxfilter.py:		A filter to label files with doxygen @addtogroup commands, and convert from
				DocOMatic format comments to doxygen format comments. Requires Python27 to be installed.
	makegroups.sh:		A tool to construct auto-groups.dox file. Requires msys or msysgit to be installed.
	filegroups.txt:		A file mapping source code paths onto groups. Used by doxyfilter.py to assign groups to source.
