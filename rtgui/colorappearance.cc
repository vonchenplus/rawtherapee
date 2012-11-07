/*
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
#include "colorappearance.h"
#include <cmath>
#include <iomanip>
#include "guiutils.h"

using namespace rtengine;
using namespace rtengine::procparams;

Colorappearance::Colorappearance () : Gtk::VBox(), FoldableToolPanel(this) {

	set_border_width(4);

	enabled = Gtk::manage (new Gtk::CheckButton (M("GENERAL_ENABLED")));
	enabled->set_active (false);
	pack_start (*enabled);


	// ------------------------ Process #1: Converting to CIECAM


	// Process 1 frame
	Gtk::Frame *p1Frame;
	// Vertical box container for the content of the Process 1 frame
	Gtk::VBox *p1VBox;

	p1Frame = Gtk::manage (new Gtk::Frame(M("TP_COLORAPP_LABEL_SCENE")) );
	p1Frame->set_border_width(0);
	p1Frame->set_label_align(0.025, 0.5);

	p1VBox = Gtk::manage ( new Gtk::VBox());
	p1VBox->set_border_width(4);
	p1VBox->set_spacing(2);

	degree  = Gtk::manage (new Adjuster (M("TP_COLORAPP_CIECAT_DEGREE"),    0.,  100.,  1.,   100.));
	if (degree->delay < 1000) degree->delay = 1000;
	degree->throwOnButtonRelease();
	degree->addAutoButton();
	degree->set_tooltip_markup (M("TP_COLORAPP_DEGREE_TOOLTIP"));
	p1VBox->pack_start (*degree);

	surrsource = Gtk::manage (new Gtk::CheckButton (M("TP_COLORAPP_SURSOURCE")));
	surrsource->set_tooltip_markup (M("TP_COLORAPP_SURSOURCE_TOOLTIP"));
	p1VBox->pack_start (*surrsource, Gtk::PACK_SHRINK);

	Gtk::HBox* wbmHBox = Gtk::manage (new Gtk::HBox ());
	wbmHBox->set_border_width (0);
	wbmHBox->set_spacing (2);
	wbmHBox->set_tooltip_markup (M("TP_COLORAPP_MODEL_TOOLTIP"));
	Gtk::Label* wbmLab = Gtk::manage (new Gtk::Label (M("TP_COLORAPP_MODEL")+":"));
	wbmHBox->pack_start (*wbmLab, Gtk::PACK_SHRINK);
	wbmodel = Gtk::manage (new MyComboBoxText ());
	wbmodel->append_text (M("TP_COLORAPP_WBRT"));
	wbmodel->append_text (M("TP_COLORAPP_WBCAM"));
	wbmodel->set_active (0);
	wbmHBox->pack_start (*wbmodel);
	p1VBox->pack_start (*wbmHBox);

	adapscen = Gtk::manage (new Adjuster (M("TP_COLORAPP_ADAPTSCENE"), 1, 4000., 1., 2000.));
	if (adapscen->delay < 1000) adapscen->delay = 1000;
	adapscen->throwOnButtonRelease();
	adapscen->set_tooltip_markup (M("TP_COLORAPP_ADAPTSCENE_TOOLTIP"));
	p1VBox->pack_start (*adapscen);

	p1Frame->add(*p1VBox);
	pack_start (*p1Frame, Gtk::PACK_EXPAND_WIDGET, 4);


	// ------------------------ Process #2: Modifying image inside CIECAM


	// Process 1 frame
	Gtk::Frame *p2Frame;
	// Vertical box container for the content of the Process 1 frame
	Gtk::VBox *p2VBox;

	p2Frame = Gtk::manage (new Gtk::Frame(M("TP_COLORAPP_LABEL_CAM02")) );
	p2Frame->set_border_width(0);
	p2Frame->set_label_align(0.025, 0.5);

	p2VBox = Gtk::manage ( new Gtk::VBox());
	p2VBox->set_border_width(4);
	p2VBox->set_spacing(2);

	Gtk::HBox* alHBox = Gtk::manage (new Gtk::HBox ());
	alHBox->set_border_width (0);
	alHBox->set_spacing (2);
	alHBox->set_tooltip_markup (M("TP_COLORAPP_ALGO_TOOLTIP"));
	Gtk::Label* alLabel = Gtk::manage (new Gtk::Label (M("TP_COLORAPP_ALGO")+":"));
	alHBox->pack_start (*alLabel, Gtk::PACK_SHRINK);
	algo = Gtk::manage (new MyComboBoxText ());
	algo->append_text (M("TP_COLORAPP_ALGO_JC"));
	algo->append_text (M("TP_COLORAPP_ALGO_JS"));
	algo->append_text (M("TP_COLORAPP_ALGO_QM"));
	algo->append_text (M("TP_COLORAPP_ALGO_ALL"));
	algo->set_active (0);
	alHBox->pack_start (*algo);
	p2VBox->pack_start (*alHBox);

	p2VBox->pack_start (*Gtk::manage (new  Gtk::HSeparator()), Gtk::PACK_EXPAND_WIDGET, 4);

	jlight = Gtk::manage (new Adjuster (M("TP_COLORAPP_LIGHT"), -100.0, 100.0, 0.1, 0.));
	if (jlight->delay < 1000) jlight->delay = 1000;
	jlight->throwOnButtonRelease();
	jlight->set_tooltip_markup (M("TP_COLORAPP_LIGHT_TOOLTIP"));
	p2VBox->pack_start (*jlight);

	qbright = Gtk::manage (new Adjuster (M("TP_COLORAPP_BRIGHT"), -100.0, 100.0, 0.1, 0.));
	if (qbright->delay < 1000) qbright->delay = 1000;
	qbright->throwOnButtonRelease();
	qbright->set_tooltip_markup (M("TP_COLORAPP_BRIGHT_TOOLTIP"));
	p2VBox->pack_start (*qbright);

	chroma = Gtk::manage (new Adjuster (M("TP_COLORAPP_CHROMA"), -100.0, 100.0, 0.1, 0.));
	if (chroma->delay < 1000) chroma->delay = 1000;
	chroma->throwOnButtonRelease();
	chroma->set_tooltip_markup (M("TP_COLORAPP_CHROMA_TOOLTIP"));
	p2VBox->pack_start (*chroma);

	rstprotection = Gtk::manage ( new Adjuster (M("TP_COLORAPP_RSTPRO"), 0., 100., 0.1, 0.) );
	if (rstprotection->delay < 1000) rstprotection->delay = 1000;
	rstprotection->throwOnButtonRelease();
	rstprotection->set_tooltip_markup (M("TP_COLORAPP_RSTPRO_TOOLTIP"));
	p2VBox->pack_start (*rstprotection);

	schroma = Gtk::manage (new Adjuster (M("TP_COLORAPP_CHROMA_S"), -100.0, 100.0, 0.1, 0.));
	if (schroma->delay < 1000) schroma->delay = 1000;
	schroma->throwOnButtonRelease();
	schroma->set_tooltip_markup (M("TP_COLORAPP_CHROMA_S_TOOLTIP"));
	p2VBox->pack_start (*schroma);

	mchroma = Gtk::manage (new Adjuster (M("TP_COLORAPP_CHROMA_M"), -100.0, 100.0, 0.1, 0.));
	if (mchroma->delay < 1000) mchroma->delay = 1000;
	mchroma->throwOnButtonRelease();
	mchroma->set_tooltip_markup (M("TP_COLORAPP_CHROMA_M_TOOLTIP"));
	p2VBox->pack_start (*mchroma);

	contrast = Gtk::manage (new Adjuster (M("TP_COLORAPP_CONTRAST"), -100.0, 100.0, 0.1, 0.));
	if (contrast->delay < 1000) contrast->delay = 1000;
	contrast->throwOnButtonRelease();
	contrast->set_tooltip_markup (M("TP_COLORAPP_CONTRAST_TOOLTIP"));
	p2VBox->pack_start (*contrast);

	qcontrast = Gtk::manage (new Adjuster (M("TP_COLORAPP_CONTRAST_Q"), -100.0, 100.0, 0.1, 0.));
	if (qcontrast->delay < 1000) qcontrast->delay = 1000;
	qcontrast->throwOnButtonRelease();
	qcontrast->set_tooltip_markup (M("TP_COLORAPP_CONTRAST_Q_TOOLTIP"));
	p2VBox->pack_start (*qcontrast);

	colorh = Gtk::manage (new Adjuster (M("TP_COLORAPP_HUE"), -100.0, 100.0, 0.1, 0.));
	if (colorh->delay < 1000) colorh->delay = 1000;
	colorh->throwOnButtonRelease();
	colorh->set_tooltip_markup (M("TP_COLORAPP_HUE_TOOLTIP"));
	p2VBox->pack_start (*colorh);

	p2Frame->add(*p2VBox);
	pack_start (*p2Frame, Gtk::PACK_EXPAND_WIDGET, 4);


	// ------------------------ Process #3: Converting back to Lab/RGB


	// Process 3 frame
	Gtk::Frame *p3Frame;
	// Vertical box container for the content of the Process 3 frame
	Gtk::VBox *p3VBox;

	p3Frame = Gtk::manage (new Gtk::Frame(M("TP_COLORAPP_LABEL_VIEWING")) ); // "Editing viewing conditions" ???
	p3Frame->set_border_width(0);
	p3Frame->set_label_align(0.025, 0.5);

	p3VBox = Gtk::manage ( new Gtk::VBox());
	p3VBox->set_border_width(4);
	p3VBox->set_spacing(2);

	adaplum = Gtk::manage (new Adjuster (M("TP_COLORAPP_ADAPTVIEWING"), 0.1,  1000., 0.1,   16.));
	if (adaplum->delay < 1000) adaplum->delay = 1000;
	adaplum->throwOnButtonRelease();
	adaplum->set_tooltip_markup (M("TP_COLORAPP_ADAPTVIEWING_TOOLTIP"));
	p3VBox->pack_start (*adaplum);

	Gtk::HBox* surrHBox = Gtk::manage (new Gtk::HBox ());
	surrHBox->set_border_width (0);
	surrHBox->set_spacing (2);
	surrHBox->set_tooltip_markup(M("TP_COLORAPP_SURROUND_TOOLTIP"));
	Gtk::Label* surrLabel = Gtk::manage (new Gtk::Label (M("TP_COLORAPP_SURROUND")+":"));
	surrHBox->pack_start (*surrLabel, Gtk::PACK_SHRINK);
	surround = Gtk::manage (new MyComboBoxText ());
	surround->append_text (M("TP_COLORAPP_SURROUND_AVER"));
	surround->append_text (M("TP_COLORAPP_SURROUND_DIM"));
	surround->append_text (M("TP_COLORAPP_SURROUND_DARK"));
	surround->append_text (M("TP_COLORAPP_SURROUND_EXDARK"));
	surround->set_active (1);
	surrHBox->pack_start (*surround);
	p3VBox->pack_start (*surrHBox);

	p3Frame->add(*p3VBox);
	pack_start (*p3Frame, Gtk::PACK_EXPAND_WIDGET, 4);


	// ------------------------ Lab Gamut control


	gamut = Gtk::manage (new Gtk::CheckButton (M("TP_COLORAPP_GAMUT")));
	gamut->set_tooltip_markup (M("TP_COLORAPP_GAMUT_TOOLTIP"));
	gamutconn = gamut->signal_toggled().connect( sigc::mem_fun(*this, &Colorappearance::gamut_toggled) );
	pack_start (*gamut, Gtk::PACK_SHRINK);


	// ------------------------ Listening events


	surrconn = surrsource->signal_toggled().connect( sigc::mem_fun(*this, &Colorappearance::surrsource_toggled) );
	enaConn = enabled->signal_toggled().connect( sigc::mem_fun(*this, &Colorappearance::enabledChanged) );
	wbmodelconn = wbmodel->signal_changed().connect ( sigc::mem_fun(*this, &Colorappearance::wbmodelChanged) );
	algoconn = algo->signal_changed().connect ( sigc::mem_fun(*this, &Colorappearance::algoChanged) );
	surroundconn = surround->signal_changed().connect ( sigc::mem_fun(*this, &Colorappearance::surroundChanged) );

	degree->setAdjusterListener  (this);
	adapscen->setAdjusterListener (this);
	adaplum->setAdjusterListener (this);
	jlight->setAdjusterListener  (this);
	qbright->setAdjusterListener  (this);
	colorh->setAdjusterListener  (this);
	chroma->setAdjusterListener  (this);
	schroma->setAdjusterListener  (this);
	mchroma->setAdjusterListener  (this);
	contrast->setAdjusterListener  (this);
	qcontrast->setAdjusterListener  (this);
	rstprotection->setAdjusterListener  (this);

	show_all();
}

bool Colorappearance::bgTTipQuery(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
	return true;
}

bool Colorappearance::srTTipQuery(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
	return true;
}

void Colorappearance::read (const ProcParams* pp, const ParamsEdited* pedited) {

	disableListener ();

	if (pedited) {
		degree->setEditedState        (pedited->colorappearance.degree ? Edited : UnEdited);
		adapscen->setEditedState      (pedited->colorappearance.adapscen ? Edited : UnEdited);
		adaplum->setEditedState       (pedited->colorappearance.adaplum ? Edited : UnEdited);
		jlight->setEditedState        (pedited->colorappearance.jlight ? Edited : UnEdited);
		qbright->setEditedState       (pedited->colorappearance.qbright ? Edited : UnEdited);
		chroma->setEditedState        (pedited->colorappearance.chroma ? Edited : UnEdited);
		schroma->setEditedState       (pedited->colorappearance.schroma ? Edited : UnEdited);
		mchroma->setEditedState       (pedited->colorappearance.mchroma ? Edited : UnEdited);
		rstprotection->setEditedState (pedited->colorappearance.rstprotection ? Edited : UnEdited);
		contrast->setEditedState      (pedited->colorappearance.contrast ? Edited : UnEdited);
		qcontrast->setEditedState     (pedited->colorappearance.qcontrast ? Edited : UnEdited);
		colorh->setEditedState        (pedited->colorappearance.colorh ? Edited : UnEdited);
		surrsource->set_inconsistent  (!pedited->colorappearance.surrsource);
		gamut->set_inconsistent       (!pedited->colorappearance.gamut);

		degree->setAutoInconsistent   (multiImage && !pedited->colorappearance.autodegree);
		enabled->set_inconsistent     (multiImage && !pedited->colorappearance.enabled);
	}

	enaConn.block (true);
	enabled->set_active (pp->colorappearance.enabled);
	enaConn.block (false);

	surroundconn.block(true);
	if (pedited && !pedited->colorappearance.surround)
		surround->set_active (4);
	else if (pp->colorappearance.surround=="Average")
		surround->set_active (0);
	else if (pp->colorappearance.surround=="Dim")
		surround->set_active (1);
	else if (pp->colorappearance.surround=="Dark")
		surround->set_active (2);
	else if (pp->colorappearance.surround=="ExtremelyDark")
		surround->set_active (3);
	surroundconn.block(false);
	// Have to be manually called to handle initial state update
	surroundChanged();

	wbmodelconn.block(true);
	if (pedited && !pedited->colorappearance.wbmodel)
		wbmodel->set_active (2);
//	else if (pp->colorappearance.wbmodel=="Equal")
//		wbmodel->set_active (0);
	else if (pp->colorappearance.wbmodel=="RawT")
		wbmodel->set_active (0);
	else if (pp->colorappearance.wbmodel=="RawTCAT02")
		wbmodel->set_active (1);
	wbmodelconn.block(false);
	// Have to be manually called to handle initial state update
	wbmodelChanged();

	algoconn.block(true);
	if (pedited && !pedited->colorappearance.algo)
		algo->set_active (4);
	else if (pp->colorappearance.algo=="JC")
		algo->set_active (0);
	else if (pp->colorappearance.algo=="JS")
		algo->set_active (1);
	else if (pp->colorappearance.algo=="QM")
		algo->set_active (2);
	else if (pp->colorappearance.algo=="ALL")
		algo->set_active (3);
	algoconn.block(false);
	// Have to be manually called to handle initial state update
	algoChanged();

	surrconn.block (true);
	surrsource->set_active (pp->colorappearance.surrsource);
	surrconn.block (false);
	gamutconn.block (true);
	gamut->set_active (pp->colorappearance.gamut);
	gamutconn.block (false);

	lastsurr=pp->colorappearance.surrsource;
	lastgamut=pp->colorappearance.gamut;

	lastEnabled = pp->colorappearance.enabled;
	lastAutoDegree = pp->colorappearance.autodegree;

	degree->setValue (pp->colorappearance.degree);
	degree->setAutoValue(pp->colorappearance.autodegree);
	adapscen->setValue (pp->colorappearance.adapscen);
	adaplum->setValue (pp->colorappearance.adaplum);
	jlight->setValue (pp->colorappearance.jlight);
	qbright->setValue (pp->colorappearance.qbright);
	chroma->setValue (pp->colorappearance.chroma);
	schroma->setValue (pp->colorappearance.schroma);
	mchroma->setValue (pp->colorappearance.mchroma);
	rstprotection->setValue (pp->colorappearance.rstprotection);
	contrast->setValue (pp->colorappearance.contrast);
	qcontrast->setValue (pp->colorappearance.qcontrast);
	colorh->setValue (pp->colorappearance.colorh);

	enableListener ();
}

void Colorappearance::write (ProcParams* pp, ParamsEdited* pedited) {

	pp->colorappearance.degree     = degree->getValue ();
	pp->colorappearance.autodegree = degree->getAutoValue ();
	pp->colorappearance.enabled    = enabled->get_active();
	pp->colorappearance.adapscen   = adapscen->getValue ();
	pp->colorappearance.adaplum    = adaplum->getValue ();
	pp->colorappearance.jlight     = jlight->getValue ();
	pp->colorappearance.qbright    = qbright->getValue ();
	pp->colorappearance.chroma     = chroma->getValue ();
	pp->colorappearance.schroma     = schroma->getValue ();
	pp->colorappearance.mchroma     = mchroma->getValue ();
	pp->colorappearance.contrast   = contrast->getValue ();
	pp->colorappearance.qcontrast   = qcontrast->getValue ();
	pp->colorappearance.colorh   = colorh->getValue ();

	pp->colorappearance.rstprotection   = rstprotection->getValue ();
	pp->colorappearance.surrsource    = surrsource->get_active();
	pp->colorappearance.gamut    = gamut->get_active();

	if (pedited) {
		pedited->colorappearance.degree        = degree->getEditedState ();
		pedited->colorappearance.adapscen      = adapscen->getEditedState ();
		pedited->colorappearance.adaplum       = adaplum->getEditedState ();
		pedited->colorappearance.jlight        = jlight->getEditedState ();
		pedited->colorappearance.qbright       = qbright->getEditedState ();
		pedited->colorappearance.chroma        = chroma->getEditedState ();
		pedited->colorappearance.schroma       = schroma->getEditedState ();
		pedited->colorappearance.mchroma       = mchroma->getEditedState ();
		pedited->colorappearance.contrast      = contrast->getEditedState ();
		pedited->colorappearance.qcontrast     = qcontrast->getEditedState ();
		pedited->colorappearance.colorh        = colorh->getEditedState ();
		pedited->colorappearance.rstprotection = rstprotection->getEditedState ();
		pedited->colorappearance.autodegree    = !degree->getAutoInconsistent();
		pedited->colorappearance.enabled       = !enabled->get_inconsistent();
		pedited->colorappearance.surround      = surround->get_active_text()!=M("GENERAL_UNCHANGED");
		pedited->colorappearance.wbmodel       = wbmodel->get_active_text()!=M("GENERAL_UNCHANGED");
		pedited->colorappearance.algo          = algo->get_active_text()!=M("GENERAL_UNCHANGED");
		pedited->colorappearance.surrsource    = !surrsource->get_inconsistent();
		pedited->colorappearance.gamut         = !gamut->get_inconsistent();
	}
	if (surround->get_active_row_number()==0)
		pp->colorappearance.surround = "Average";
	else if (surround->get_active_row_number()==1)
		pp->colorappearance.surround = "Dim";
	else if (surround->get_active_row_number()==2)
		pp->colorappearance.surround = "Dark";
	else if (surround->get_active_row_number()==3)
		pp->colorappearance.surround = "ExtremelyDark";

//	if (wbmodel->get_active_row_number()==0)
//		pp->colorappearance.wbmodel = "Equal";
	if (wbmodel->get_active_row_number()==0)
		pp->colorappearance.wbmodel = "RawT";
	else if (wbmodel->get_active_row_number()==1)
		pp->colorappearance.wbmodel = "RawTCAT02";

	if (algo->get_active_row_number()==0)
		pp->colorappearance.algo = "JC";
	else if (algo->get_active_row_number()==1)
		pp->colorappearance.algo = "JS";
	else if (algo->get_active_row_number()==2)
		pp->colorappearance.algo = "QM";
	else if (algo->get_active_row_number()==3)
		pp->colorappearance.algo = "ALL";

}

void Colorappearance::surrsource_toggled () {

	if (batchMode) {
		if (surrsource->get_inconsistent()) {
			surrsource->set_inconsistent (false);
			surrconn.block (true);
			surrsource->set_active (false);
			surrconn.block (false);
		}
		else if (lastsurr)
			surrsource->set_inconsistent (true);

		lastsurr = surrsource->get_active ();
	}

	if (listener) {
		if (surrsource->get_active ())
			listener->panelChanged (EvCATsurr, M("GENERAL_ENABLED"));
		else
			listener->panelChanged (EvCATsurr, M("GENERAL_DISABLED"));
	}
}

void Colorappearance::gamut_toggled () {

	if (batchMode) {
		if (gamut->get_inconsistent()) {
			gamut->set_inconsistent (false);
			gamutconn.block (true);
			gamut->set_active (false);
			gamutconn.block (false);
		}
		else if (lastgamut)
			gamut->set_inconsistent (true);

		lastgamut = gamut->get_active ();
	}

	if (listener) {
		if (gamut->get_active ())
			listener->panelChanged (EvCATgamut, M("GENERAL_ENABLED"));
		else
			listener->panelChanged (EvCATgamut, M("GENERAL_DISABLED"));
	}
}


void Colorappearance::setDefaults (const ProcParams* defParams, const ParamsEdited* pedited) {

	degree->setDefault (defParams->colorappearance.degree);
	adapscen->setDefault (defParams->colorappearance.adapscen);
	adaplum->setDefault (defParams->colorappearance.adaplum);
	jlight->setDefault (defParams->colorappearance.jlight);
	qbright->setDefault (defParams->colorappearance.qbright);
	chroma->setDefault (defParams->colorappearance.chroma);
	schroma->setDefault (defParams->colorappearance.schroma);
	mchroma->setDefault (defParams->colorappearance.mchroma);
	rstprotection->setDefault (defParams->colorappearance.rstprotection);
	contrast->setDefault (defParams->colorappearance.contrast);
	qcontrast->setDefault (defParams->colorappearance.qcontrast);
	colorh->setDefault (defParams->colorappearance.colorh);

	if (pedited) {
		degree->setDefaultEditedState (pedited->colorappearance.degree ? Edited : UnEdited);
		adapscen->setDefaultEditedState (pedited->colorappearance.adapscen ? Edited : UnEdited);
		adaplum->setDefaultEditedState (pedited->colorappearance.adaplum ? Edited : UnEdited);
		jlight->setDefaultEditedState (pedited->colorappearance.jlight ? Edited : UnEdited);
		qbright->setDefaultEditedState (pedited->colorappearance.qbright ? Edited : UnEdited);
		chroma->setDefaultEditedState (pedited->colorappearance.chroma ? Edited : UnEdited);
		schroma->setDefaultEditedState (pedited->colorappearance.schroma ? Edited : UnEdited);
		mchroma->setDefaultEditedState (pedited->colorappearance.mchroma ? Edited : UnEdited);
		rstprotection->setDefaultEditedState (pedited->colorappearance.rstprotection ? Edited : UnEdited);
		contrast->setDefaultEditedState (pedited->colorappearance.contrast ? Edited : UnEdited);
		qcontrast->setDefaultEditedState (pedited->colorappearance.qcontrast ? Edited : UnEdited);
		colorh->setDefaultEditedState (pedited->colorappearance.colorh ? Edited : UnEdited);
		
	}
	else {
		degree->setDefaultEditedState (Irrelevant);
		adapscen->setDefaultEditedState (Irrelevant);
		adaplum->setDefaultEditedState (Irrelevant);
		jlight->setDefaultEditedState (Irrelevant);
		qbright->setDefaultEditedState (Irrelevant);
		chroma->setDefaultEditedState (Irrelevant);
		schroma->setDefaultEditedState (Irrelevant);
		mchroma->setDefaultEditedState (Irrelevant);
		contrast->setDefaultEditedState (Irrelevant);
		qcontrast->setDefaultEditedState (Irrelevant);
		rstprotection->setDefaultEditedState (Irrelevant);
		colorh->setDefaultEditedState (Irrelevant);
		
	}
}

void Colorappearance::adjusterChanged (Adjuster* a, double newval) {

	if (listener && (multiImage||enabled->get_active()) ) {
		if(a==degree)
			listener->panelChanged (EvCATDegree, a->getTextValue());
		else if(a==adapscen)
			listener->panelChanged (EvCATAdapscen, a->getTextValue());
		else if(a==adaplum)
			listener->panelChanged (EvCATAdapLum, a->getTextValue());
		else if(a==jlight)
			listener->panelChanged (EvCATJLight, a->getTextValue());
		else if(a==qbright)
			listener->panelChanged (EvCATQbright, a->getTextValue());
		else if(a==chroma)
			listener->panelChanged (EvCATChroma, a->getTextValue());
		else if(a==schroma)
			listener->panelChanged (EvCATSChroma, a->getTextValue());
		else if(a==mchroma)
			listener->panelChanged (EvCATMChroma, a->getTextValue());
		else if(a==rstprotection)
			listener->panelChanged (EvCATRstpro, a->getTextValue());
		else if(a==contrast)
			listener->panelChanged (EvCATContrast, a->getTextValue());
		else if(a==colorh)
			listener->panelChanged (EvCAThue, a->getTextValue());
		else if(a==qcontrast)
			listener->panelChanged (EvCATQContrast, a->getTextValue());
			
	}
}

void Colorappearance::adjusterAutoToggled (Adjuster* a, bool newval) {

	if (multiImage) {
		if (degree->getAutoInconsistent()) {
			degree->setAutoInconsistent(false);
			degree->setAutoValue(false);
		}
		else if (lastAutoDegree)
			degree->setAutoInconsistent(true);

		lastAutoDegree = degree->getAutoValue();
	}

	if (listener && (multiImage||enabled->get_active()) ) {
		if(a==degree) {
			if (degree->getAutoInconsistent())
				listener->panelChanged (EvCATAutoDegree, M("GENERAL_UNCHANGED"));
			else if (degree->getAutoValue())
				listener->panelChanged (EvCATAutoDegree, M("GENERAL_ENABLED"));
			else
				listener->panelChanged (EvCATAutoDegree, M("GENERAL_DISABLED"));
		}
	}
}

void Colorappearance::enabledChanged () {

	if (multiImage) {
		if (enabled->get_inconsistent()) {
			enabled->set_inconsistent (false);
			enaConn.block (true);
			enabled->set_active (false);
			enaConn.block (false);
		}
		else if (lastEnabled)
			enabled->set_inconsistent (true);

		lastEnabled = enabled->get_active ();
	}

	if (listener) {
		if (enabled->get_inconsistent())
			listener->panelChanged (EvCATEnabled, M("GENERAL_UNCHANGED"));
		else if (enabled->get_active ())
			listener->panelChanged (EvCATEnabled, M("GENERAL_ENABLED"));
		else
			listener->panelChanged (EvCATEnabled, M("GENERAL_DISABLED"));
	}
}

void Colorappearance::surroundChanged () {

	if (listener && (multiImage||enabled->get_active()) ) {
		listener->panelChanged (EvCATMethodsur, surround->get_active_text ());
	}
}

void Colorappearance::wbmodelChanged () {

	if (listener && (multiImage||enabled->get_active()) ) {
		listener->panelChanged (EvCATMethodWB, wbmodel->get_active_text ());
	}
}

void Colorappearance::algoChanged () {

	if ( algo->get_active_row_number()==0 ) {
		contrast->show();
		rstprotection->show();
		qcontrast->hide();
		jlight->show();
		mchroma->hide();
		chroma->show();
		schroma->hide();
		qbright->hide();
		colorh->hide();
	}
	else if ( algo->get_active_row_number()==1 ) {
		rstprotection->hide();
		contrast->show();
		qcontrast->hide();
		jlight->show();
		mchroma->hide();
		chroma->hide();
		schroma->show();
		qbright->hide();
		colorh->hide();
	}
	else if ( algo->get_active_row_number()==2 ) {
		contrast->hide();
		rstprotection->hide();
		qcontrast->show();
		jlight->hide();
		mchroma->show();
		chroma->hide();
		schroma->hide();
		qbright->show();
		colorh->hide();
	}
	else if ( algo->get_active_row_number()>=3 ) {  // ">=3" because everything has to be visible with the "(unchanged)" option too
		contrast->show();
		rstprotection->show();
		qcontrast->show();
		jlight->show();
		mchroma->show();
		chroma->show();
		schroma->show();
		qbright->show();
		colorh->show();
	}

	if (listener && (multiImage||enabled->get_active()) ) {
		listener->panelChanged (EvCATMethodalg, algo->get_active_text ());
	}
}

void Colorappearance::setBatchMode (bool batchMode) {

	ToolPanel::setBatchMode (batchMode);

	degree->showEditedCB ();
	adapscen->showEditedCB ();
	adaplum->showEditedCB ();
	jlight->showEditedCB ();
	qbright->showEditedCB ();
	chroma->showEditedCB ();
	schroma->showEditedCB ();
	mchroma->showEditedCB ();
	rstprotection->showEditedCB ();
	contrast->showEditedCB ();
	qcontrast->showEditedCB ();
	colorh->showEditedCB ();

	surround->append_text (M("GENERAL_UNCHANGED"));
	wbmodel->append_text (M("GENERAL_UNCHANGED"));
	algo->append_text (M("GENERAL_UNCHANGED"));
	
}

void Colorappearance::setAdjusterBehavior (bool degreeadd, bool adapscenadd, bool adaplumadd, bool jlightadd, bool chromaadd, bool contrastadd, bool rstprotectionadd, bool qbrightadd, bool qcontrastadd, bool schromaadd, bool mchromaadd, bool colorhadd) {

	degree->setAddMode(degreeadd);
	adapscen->setAddMode(adapscenadd);
	adaplum->setAddMode(adaplumadd);
	jlight->setAddMode(jlightadd);
	qbright->setAddMode(qbrightadd);
	chroma->setAddMode(chromaadd);
	schroma->setAddMode(schromaadd);
	mchroma->setAddMode(mchromaadd);
	rstprotection->setAddMode(rstprotectionadd);
	contrast->setAddMode(contrastadd);
	qcontrast->setAddMode(qcontrastadd);
	colorh->setAddMode(colorhadd);
	
}

void Colorappearance::trimValues (rtengine::procparams::ProcParams* pp) {

	degree->trimValue(pp->colorappearance.degree);
	adapscen->trimValue(pp->colorappearance.adapscen);
	adaplum->trimValue(pp->colorappearance.adaplum);
	jlight->trimValue(pp->colorappearance.jlight);
	qbright->trimValue(pp->colorappearance.qbright);
	chroma->trimValue(pp->colorappearance.chroma);
	schroma->trimValue(pp->colorappearance.schroma);
	mchroma->trimValue(pp->colorappearance.mchroma);
	rstprotection->trimValue(pp->colorappearance.rstprotection);
	contrast->trimValue(pp->colorappearance.contrast);
	qcontrast->trimValue(pp->colorappearance.qcontrast);
	colorh->trimValue(pp->colorappearance.colorh);

}