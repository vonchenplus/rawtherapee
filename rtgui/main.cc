/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
// generated 2004/6/3 19:15:32 CEST by gabor@darkstar.(none)
// using glademm V2.5.0
//
// newer (non customized) versions of this file go to raw.cc_new

// This file is for your program, I won't touch it again!

//#include <config.h>
#include <gtkmm.h>
#include <giomm.h>
#include <iostream>
#include <rtwindow.h>
#include <string.h>
#include <stdlib.h>
#include <options.h>

#ifndef WIN32
#include <config.h>
#include <glibmm/fileutils.h>
#include <glib.h>
#include <glib/gstdio.h>
#endif

#include <safegtk.h>

extern Options options;

// stores path to data files
Glib::ustring argv0;
Glib::ustring argv1;

int main(int argc, char **argv)
{

   Glib::ustring argv0_, argv1_;

#ifdef WIN32

   WCHAR exnameU[512] = {0};
   char exname[512] = {0};
   GetModuleFileNameW (NULL, exnameU, 512);
   WideCharToMultiByte(CP_UTF8,0,exnameU,-1,exname,512,0,0 );
   argv0_ = exname;

   // get the path where the rawtherapee executable is stored
   argv0 = Glib::path_get_dirname(argv0_);

#else

   // get the path to data (defined in config.h which is generated by cmake)
   argv0 = DATA_SEARCH_PATH;
   // check if path exists, otherwise revert back to behavior similar to windows
   try {
      Glib::Dir dir(DATA_SEARCH_PATH);
   } catch (Glib::FileError) {
       argv0_ = argv[0];
       argv0  = Glib::path_get_dirname(argv0_);
   }

#endif


    if (argc>1)
        argv1_ = argv[1];
    else
        argv1_ = "";

    argv1 = safe_filename_to_utf8 (argv1_);

   Glib::thread_init();
   gdk_threads_init();
   Gio::init ();

   Options::load ();

#ifndef _WIN32
   // Move the old path to the new one if the new does not exist
   if (safe_file_test(Glib::build_filename(options.rtdir,"cache"), Glib::FILE_TEST_IS_DIR) && !safe_file_test(options.cacheBaseDir, Glib::FILE_TEST_IS_DIR))
       safe_g_rename(Glib::build_filename(options.rtdir,"cache"), options.cacheBaseDir);
#endif

//   Gtk::RC::add_default_file (argv0+"/themes/"+options.theme);
   if (!options.useSystemTheme)
   {
       std::vector<Glib::ustring> rcfiles;
       rcfiles.push_back (argv0+"/themes/"+options.theme+".gtkrc");
   	   if (options.slimUI)
           rcfiles.push_back (argv0+"/themes/slim");
       // Set the font face and size
       Gtk::RC::parse_string (Glib::ustring::compose(
          "style \"clearlooks-default\" { font_name = \"%1\" }", options.font));
       Gtk::RC::set_default_files (rcfiles);
   }
   Gtk::Main m(&argc, &argv);

   RTWindow *rtWindow = new class RTWindow();
   gdk_threads_enter ();
   m.run(*rtWindow);
   gdk_threads_leave ();
   delete rtWindow;
   rtengine::cleanup();
   return 0;
}




