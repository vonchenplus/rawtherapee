
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
#include <filepanel.h>
#include <rtwindow.h>
#include <safegtk.h>

int fbinit (void* data) {

    gdk_threads_enter ();
    ((FilePanel*)data)->init ();
    gdk_threads_leave ();

    return 0;
}

FilePanel::FilePanel () : parent(NULL) {

    isloading = false;
    dirpaned = Gtk::manage ( new Gtk::HPaned () );
    dirpaned->set_position (options.dirBrowserWidth);

    dirBrowser = new DirBrowser ();
    placesBrowser = new PlacesBrowser ();
    recentBrowser = new RecentBrowser ();

    placespaned = Gtk::manage ( new Gtk::VPaned () );
    placespaned->set_size_request(50,100);
    placespaned->set_position (options.dirBrowserHeight);

    Gtk::VBox* obox = Gtk::manage (new Gtk::VBox ());
    obox->pack_start (*recentBrowser, Gtk::PACK_SHRINK, 4);
    obox->pack_start (*dirBrowser);

    placespaned->pack1 (*placesBrowser, false, true);
    placespaned->pack2 (*obox, true, true);

    dirpaned->pack1 (*placespaned, false, true);

    tpc = new BatchToolPanelCoordinator (this);
    fileCatalog = new FileCatalog (tpc->coarse, tpc->getToolBar());
    ribbonPane = Gtk::manage ( new Gtk::Paned() );
    ribbonPane->add(*fileCatalog);
    ribbonPane->set_size_request(50,150);
    dirpaned->pack2 (*ribbonPane, true, true);

    placesBrowser->setDirBrowserRemoteInterface (dirBrowser);
    recentBrowser->setDirBrowserRemoteInterface (dirBrowser);
    dirBrowser->addDirSelectionListener (fileCatalog);
    dirBrowser->addDirSelectionListener (recentBrowser);
    dirBrowser->addDirSelectionListener (placesBrowser);
    fileCatalog->setFileSelectionListener (this);

    rightBox = Gtk::manage ( new Gtk::HBox () );
    rightBox->set_size_request(50,100);
    rightNotebook = Gtk::manage ( new Gtk::Notebook () );
    //Gtk::VBox* taggingBox = Gtk::manage ( new Gtk::VBox () );

    history = new History (false);

    tpc->addPParamsChangeListener (history);
    history->setProfileChangeListener (tpc);

    Gtk::ScrolledWindow* sFilterPanel = Gtk::manage ( new Gtk::ScrolledWindow() );
	filterPanel = Gtk::manage ( new FilterPanel () );
	sFilterPanel->add (*filterPanel);
	sFilterPanel->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	fileCatalog->setFilterPanel (filterPanel);
    fileCatalog->setImageAreaToolListener (tpc);

    //------------------

    rightNotebook->set_tab_pos (Gtk::POS_LEFT);

    Gtk::Label* devLab = Gtk::manage ( new Gtk::Label (M("MAIN_TAB_DEVELOP")) );
    devLab->set_angle (90);
    Gtk::Label* filtLab = Gtk::manage ( new Gtk::Label (M("MAIN_TAB_FILTER")) );
    filtLab->set_angle (90);
    //Gtk::Label* tagLab = Gtk::manage ( new Gtk::Label (M("MAIN_TAB_TAGGING")) );
    //tagLab->set_angle (90);

    tpcPaned = Gtk::manage ( new Gtk::VPaned () );
    tpcPaned->pack1 (*tpc->toolPanelNotebook, false, true);
    tpcPaned->pack2 (*history, true, true);

    rightNotebook->append_page (*tpcPaned, *devLab);
    rightNotebook->append_page (*sFilterPanel, *filtLab);
    //rightNotebook->append_page (*taggingBox, *tagLab); commented out: currently the tab is empty ...

    rightBox->pack_start (*rightNotebook);

    pack1(*dirpaned, true, true);
    pack2(*rightBox, false, true);

    fileCatalog->setFileSelectionChangeListener (tpc);

    fileCatalog->setFileSelectionListener (this);
    g_idle_add (fbinit, this);

    show_all ();
}

void FilePanel::setAspect () {
	int winW, winH;
	parent->get_size(winW, winH);
	placespaned->set_position(options.dirBrowserHeight);
	dirpaned->set_position(options.dirBrowserWidth);
	tpcPaned->set_position(options.browserToolPanelHeight);
	set_position(winW - options.browserToolPanelWidth);
}

void FilePanel::init () {
  
    dirBrowser->fillDirTree ();
    placesBrowser->refreshPlacesList ();

    if (argv1!="" && safe_file_test (argv1, Glib::FILE_TEST_IS_DIR))
        dirBrowser->open (argv1);
    else {
        if (options.startupDir==STARTUPDIR_HOME) 
            dirBrowser->open (Glib::get_home_dir());
        else if (options.startupDir==STARTUPDIR_CURRENT)
            dirBrowser->open (argv0);
        else if (options.startupDir==STARTUPDIR_CUSTOM || options.startupDir==STARTUPDIR_LAST) {
            if (options.startupPath.length() && safe_file_test(options.startupPath, Glib::FILE_TEST_EXISTS) && safe_file_test(options.startupPath, Glib::FILE_TEST_IS_DIR))
                dirBrowser->open (options.startupPath);
            else {
                // Fallback option if the path is empty or the folder doesn't exist
                dirBrowser->open (Glib::get_home_dir());
            }
        }
    }
} 

bool FilePanel::fileSelected (Thumbnail* thm) {

    if (!parent)
        return false;

    // try to open the file
  //  fileCatalog->setEnabled (false);
    if (isloading)
        return false;

    isloading = true;
    ProgressConnector<rtengine::InitialImage*> *ld = new ProgressConnector<rtengine::InitialImage*>();
    ld->startFunc (sigc::bind(sigc::ptr_fun(&rtengine::InitialImage::load), thm->getFileName (), thm->getType()==FT_Raw, &error, parent->getProgressListener()),
   		           sigc::bind(sigc::mem_fun(*this,&FilePanel::imageLoaded), thm, ld) );
    return true;
}
bool FilePanel::imageLoaded( Thumbnail* thm, ProgressConnector<rtengine::InitialImage*> *pc ){

	if (pc->returnValue() && thm) {
               
        if (options.tabbedUI) {
            EditorPanel* epanel = Gtk::manage (new EditorPanel ());
            parent->addEditorPanel (epanel,Glib::path_get_basename (thm->getFileName()));
            epanel->open(thm, pc->returnValue() );
        } else {
            parent->SetEditorCurrent();
            parent->epanel->open(thm, pc->returnValue() );                     
        }

	} else {
		Glib::ustring msg_ = Glib::ustring("<b>") + M("MAIN_MSG_CANNOTLOAD") + " \"" + thm->getFileName() + "\" .\n</b>";
		Gtk::MessageDialog msgd (msg_, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		msgd.run ();
	}
	delete pc;
        
	parent->setProgress(0.);
	parent->setProgressStr("");
        isloading = false;

	return false; // MUST return false from idle function
}

void FilePanel::saveOptions () { 

	int winW, winH;
	parent->get_size(winW, winH);
    options.dirBrowserWidth = dirpaned->get_position ();
    options.dirBrowserHeight = placespaned->get_position ();
    options.browserToolPanelWidth = winW - get_position();
    options.browserToolPanelHeight = tpcPaned->get_position ();
     if (options.startupDir==STARTUPDIR_LAST && fileCatalog->lastSelectedDir ()!="")
        options.startupPath = fileCatalog->lastSelectedDir ();
    fileCatalog->closeDir ();
}

void FilePanel::open (const Glib::ustring& d) {

    if (safe_file_test (d, Glib::FILE_TEST_IS_DIR))
        dirBrowser->open (d.c_str());
    else if (safe_file_test (d, Glib::FILE_TEST_EXISTS))
        dirBrowser->open (Glib::path_get_dirname(d), Glib::path_get_basename(d));
}

bool FilePanel::addBatchQueueJobs ( std::vector<BatchQueueEntry*> &entries ) {

    if (parent)
        parent->addBatchQueueJobs (entries);
	return true;
}

void FilePanel::optionsChanged () {

    tpc->optionsChanged ();
    fileCatalog->refreshAll ();
}

bool FilePanel::handleShortcutKey (GdkEventKey* event) {

    bool ctrl = event->state & GDK_CONTROL_MASK;
    bool shift = event->state & GDK_SHIFT_MASK;
    
    if (!ctrl) {
        switch(event->keyval) {
        }
    }
    else {
        switch (event->keyval) {
        }
    }
    
    if(tpc->getToolBar()->handleShortcutKey(event))
        return true;
    
    if(fileCatalog->handleShortcutKey(event))
        return true;

    return false;
}
