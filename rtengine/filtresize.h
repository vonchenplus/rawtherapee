/*
 * filtresize.h
 *
 *  Created on: Sep 23, 2010
 *      Author: gabor
 */

#ifndef FILTRESIZE_H_
#define FILTRESIZE_H_

#include "filter.h"

//
// R e s i z e   f i l t e r
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//

namespace rtengine {

class ResizeFilterDescriptor : public FilterDescriptor {

	public:
        ResizeFilterDescriptor ();
    	void getDefaultParameters (ProcParams& defProcParams) const;
		void createAndAddToList (Filter* tail) const;
};

extern ResizeFilterDescriptor resizeFilterDescriptor;

class ResizeFilter : public Filter {

        double getResizeScale ();

        void bicubic  (MultiImage* sourceImage, MultiImage* targetImage);
        void bilinear (MultiImage* sourceImage, MultiImage* targetImage);
        void average  (MultiImage* sourceImage, MultiImage* targetImage);
        void nearest  (MultiImage* sourceImage, MultiImage* targetImage);

    public:
        ResizeFilter ();

    	void      process (const std::set<ProcEvent>& events, MultiImage* sourceImage, MultiImage* targetImage, Buffer<float>* buffer);
        double    getScale ();
        double    getTargetScale (int skip);
        ImageView calculateSourceImageView (const ImageView& requestedImView);
        Dim       getFullImageSize ();
        void      reverseTransPoint (int x, int y, int& xv, int& yv);
};

}
#endif /* FILTRESIZE_H_ */
