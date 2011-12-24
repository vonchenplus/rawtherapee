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
#include "rgbcurves.h"
#include <iomanip>

using namespace rtengine;
using namespace rtengine::procparams;

RGBCurves::RGBCurves () : Gtk::VBox(), FoldableToolPanel(this) {

	curveEditorG = new CurveEditorGroup ();
	curveEditorG->setCurveListener (this);
	curveEditorG->setColorProvider (this);

	Rshape = (DiagonalCurveEditor*)curveEditorG->addCurve(CT_Diagonal, "R");
	Gshape = (DiagonalCurveEditor*)curveEditorG->addCurve(CT_Diagonal, "G");
	Bshape = (DiagonalCurveEditor*)curveEditorG->addCurve(CT_Diagonal, "B");

	// This will add the reset button at the end of the curveType buttons
	curveEditorG->curveListComplete();

	pack_start (*curveEditorG, Gtk::PACK_SHRINK, 4);
	
}

RGBCurves::~RGBCurves () {
	delete curveEditorG;
}

void RGBCurves::read (const ProcParams* pp, const ParamsEdited* pedited) {

    disableListener ();

    if (pedited) {
        Rshape->setUnChanged (!pedited->rgbCurves.rcurve);
		Gshape->setUnChanged (!pedited->rgbCurves.gcurve);
        Bshape->setUnChanged (!pedited->rgbCurves.bcurve);
    }

    Rshape->setCurve         (pp->rgbCurves.rcurve);
	Gshape->setCurve         (pp->rgbCurves.gcurve);
    Bshape->setCurve         (pp->rgbCurves.bcurve);

    // Open up the first curve if selected
    bool active = Rshape->openIfNonlinear();
    if (!active) Gshape->openIfNonlinear();
    if (!active) Bshape->openIfNonlinear();

    enableListener ();
}

void RGBCurves::write (ProcParams* pp, ParamsEdited* pedited) {

    pp->rgbCurves.rcurve         = Rshape->getCurve ();
	pp->rgbCurves.gcurve         = Gshape->getCurve ();
    pp->rgbCurves.bcurve         = Bshape->getCurve ();

    if (pedited) {
        pedited->rgbCurves.rcurve    = !Rshape->isUnChanged ();
		pedited->rgbCurves.gcurve    = !Gshape->isUnChanged ();
        pedited->rgbCurves.bcurve    = !Bshape->isUnChanged ();
    }
}


/*
 * Curve listener
 *
 * If more than one curve has been added, the curve listener is automatically
 * set to 'multi=true', and send a pointer of the modified curve in a parameter
 */
void RGBCurves::curveChanged (CurveEditor* ce) {

    if (listener) {
    	if (ce == Rshape)
        	listener->panelChanged (EvRGBrCurve, M("HISTORY_CUSTOMCURVE"));
    	if (ce == Gshape)
        	listener->panelChanged (EvRGBgCurve, M("HISTORY_CUSTOMCURVE"));
    	if (ce == Bshape)
        	listener->panelChanged (EvRGBbCurve, M("HISTORY_CUSTOMCURVE"));
	}
}


//!!! TODO: change
void RGBCurves::colorForValue (double valX, double valY) {

	CurveEditor* ce = curveEditorG->getDisplayedCurve();

	if (ce == Rshape) {         // L = f(L)
		red = (double)valY;
		green = (double)valY;
		blue = (double)valY;
	}
	else if (ce == Gshape) {    // a = f(a)
		// TODO: To be implemented
		red = (double)valY;
		green = (double)valY;
		blue = (double)valY;
	}
	else if (ce == Bshape) {    // b = f(b)
		red = (double)valY;
		green = (double)valY;
		blue = (double)valY;
	}
	else {
		printf("Error: no curve displayed!\n");
	}

}

void RGBCurves::setBatchMode (bool batchMode) {

    ToolPanel::setBatchMode (batchMode);
    curveEditorG->setBatchMode (batchMode);
}


void RGBCurves::updateCurveBackgroundHistogram (LUTu & histToneCurve, LUTu & histLCurve, LUTu & histRed, LUTu & histGreen, LUTu & histBlue, LUTu & histLuma) {

    //Rshape->updateBackgroundHistogram (histRed);
    //Gshape->updateBackgroundHistogram (histGreen);
    //Bshape->updateBackgroundHistogram (histBlue);
}
