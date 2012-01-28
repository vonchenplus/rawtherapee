/*
 *  This file is part of RawTherapee.
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
 *
 *  2010 Ilya Popov <ilia_popov@rambler.ru>
 *  2012 Emil Martinec <ejmartin@uchicago.edu>
 */

#ifndef CPLX_WAVELET_DEC_H_INCLUDED
#define CPLX_WAVELET_DEC_H_INCLUDED

#include <cstddef>
#include <math.h>


#include "cplx_wavelet_level.h"
#include "cplx_wavelet_filter_coeffs.h"

namespace rtengine {


// %%%%%%%%%%%%%%%%%%%%%%%%%%%

template <typename A, typename B>
void copy_out(A ** a, B * b, size_t datalen)
{
	for (size_t j=0; j<datalen; j++) {
		b[j] = static_cast<B> (0.25*(a[0][j]+a[1][j]+a[2][j]+a[3][j]));
	}
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%


class cplx_wavelet_decomposition
{
public:

    typedef float internal_type;

private:

    static const int maxlevels = 8;//should be greater than any conceivable order of decimation
    
    int lvltot;
    size_t m_w, m_h;//dimensions
    size_t m_w1, m_h1;
	
	int first_lev_len, first_lev_offset;
	float *first_lev_anal;
	float *first_lev_synth;
	
	int wavfilt_len, wavfilt_offset;
	float *wavfilt_anal;
	float *wavfilt_synth;
	
	int testfilt_len, testfilt_offset;
	float *testfilt_anal;
	float *testfilt_synth;

    cplx_wavelet_level<internal_type> * dual_tree_coeffs[maxlevels][4];//m_c in old code
    
public:

    template<typename E>
    cplx_wavelet_decomposition(E * src, int width, int height, int maxlvl);
    
    ~cplx_wavelet_decomposition();
    
    template<typename E>
    void reconstruct(E * dst);
	
		
};
	
	
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	
	template<typename E>
	cplx_wavelet_decomposition::cplx_wavelet_decomposition(E * src, int width, int height, int maxlvl)
	: lvltot(0), m_w(width), m_h(height), m_w1(0), m_h1(0)
	{
		m_w1 = width;
		m_h1 = height;
		
		//initialize wavelet filters
		
		first_lev_len = Kingsbury_len;
		first_lev_offset = Kingsbury_offset;
		first_lev_anal = new float[4*first_lev_len];
		first_lev_synth = new float[4*first_lev_len];

		for (int n=0; n<2; n++) {
			for (int m=0; m<2; m++) {
				for (int i=0; i<first_lev_len; i++) {
					first_lev_anal[first_lev_len*(2*n+m)+i]  = Kingsbury_anal[n][m][i];
					first_lev_synth[first_lev_len*(2*n+m)+i] = Kingsbury_anal[n][m][first_lev_len-1-i];
				}
			}
		}
		
		wavfilt_len = Kingsbury_len;
		wavfilt_offset = Kingsbury_offset;
		wavfilt_anal = new float[4*wavfilt_len];
		wavfilt_synth = new float[4*wavfilt_len];
		
		for (int n=0; n<2; n++) {
			for (int m=0; m<2; m++) {
				for (int i=0; i<wavfilt_len; i++) {
					wavfilt_anal[wavfilt_len*(2*n+m)+i]  = Kingsbury_anal[n][m][i];
					wavfilt_synth[wavfilt_len*(2*n+m)+i] = Kingsbury_anal[n][m][first_lev_len-1-i];
				}
			}
		}
		
		
		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 

		
		// data structure is dual_tree_coeffs[scale][2*n+m=2*(Re/Im)+dir][channel={lo,hi1,hi2,hi3}][pixel_array]
		
		for (int n=0; n<2; n++) {
			for (int m=0; m<2; m++) {
				float padding = 0;//pow(2, maxlvl);//must be a multiple of two
				dual_tree_coeffs[0][2*n+m] = new cplx_wavelet_level<internal_type>(src, padding, m_w, m_h, first_lev_anal+first_lev_len*2*n, \
																				   first_lev_anal+first_lev_len*2*m, first_lev_len, first_lev_offset);
				lvltot=1;
				while(lvltot < maxlvl) {
					dual_tree_coeffs[lvltot][2*n+m] = new cplx_wavelet_level<internal_type>(dual_tree_coeffs[lvltot-1][2*n+m]->lopass()/*lopass*/, 0/*no padding*/, \
																							dual_tree_coeffs[lvltot-1][2*n+m]->width(), \
																							dual_tree_coeffs[lvltot-1][2*n+m]->height(), \
																							wavfilt_anal+wavfilt_len*2*n, wavfilt_anal+wavfilt_len*2*m, wavfilt_len, wavfilt_offset);
					lvltot++;
				}
			}
		}
		
		
		//rotate detail coefficients
		float root2 = sqrt(2);
		for (int lvl=0; lvl<lvltot; lvl++) {
			int Wlvl = dual_tree_coeffs[lvl][0]->width();
			int Hlvl = dual_tree_coeffs[lvl][0]->height();
			for (int i=0; i<Wlvl*Hlvl; i++) {//pixel
				for (int m=1; m<4; m++) {//detail coefficients only
					float wavtmp = (dual_tree_coeffs[lvl][0]->wavcoeffs[m][i] + dual_tree_coeffs[lvl][3]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][3]->wavcoeffs[m][i] = (dual_tree_coeffs[lvl][0]->wavcoeffs[m][i] - dual_tree_coeffs[lvl][3]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][0]->wavcoeffs[m][i] = wavtmp;
					
					wavtmp = (dual_tree_coeffs[lvl][1]->wavcoeffs[m][i] + dual_tree_coeffs[lvl][2]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][2]->wavcoeffs[m][i] = (dual_tree_coeffs[lvl][1]->wavcoeffs[m][i] - dual_tree_coeffs[lvl][2]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][1]->wavcoeffs[m][i] = wavtmp;
				}
			}
		}
		
	}
	
	/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
	/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
	
	
	/* function y=reconstruct(w,J,Fsf,sf) */
	
	template<typename E>
	void cplx_wavelet_decomposition::reconstruct(E * dst) { 
		
		// data structure is wavcoeffs[scale][2*n+m=2*(Re/Im)+dir][channel={lo,hi1,hi2,hi3}][pixel_array]
		
		//rotate detail coefficients
		float root2 = sqrt(2);
		for (int lvl=0; lvl<lvltot; lvl++) {
			int Wlvl = dual_tree_coeffs[lvl][0]->width();
			int Hlvl = dual_tree_coeffs[lvl][0]->height();
			for (int i=0; i<Wlvl*Hlvl; i++) {//pixel
				for (int m=1; m<4; m++) {//detail coefficients only
					float wavtmp = (dual_tree_coeffs[lvl][0]->wavcoeffs[m][i] + dual_tree_coeffs[lvl][3]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][3]->wavcoeffs[m][i] = (dual_tree_coeffs[lvl][0]->wavcoeffs[m][i] - dual_tree_coeffs[lvl][3]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][0]->wavcoeffs[m][i] = wavtmp;
					
					wavtmp = (dual_tree_coeffs[lvl][1]->wavcoeffs[m][i] + dual_tree_coeffs[lvl][2]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][2]->wavcoeffs[m][i] = (dual_tree_coeffs[lvl][1]->wavcoeffs[m][i] - dual_tree_coeffs[lvl][2]->wavcoeffs[m][i])/root2;
					dual_tree_coeffs[lvl][1]->wavcoeffs[m][i] = wavtmp;
				}
			}
		}
		
		//y = ConstantArray[0, {vsizetmp, hsizetmp}];
		internal_type ** tmp = new internal_type *[4];
		for (int i=0; i<4; i++) {
			tmp[i] = new internal_type[m_w*m_h];
		}
		
		for (int n=0; n<2; n++) {
			for (int m=0; m<2; m++) {
				for (int lvl=lvltot-1; lvl>0; lvl--) {
					dual_tree_coeffs[lvl][2*n+m]->reconstruct_level(dual_tree_coeffs[lvl-1][2*n+m]->wavcoeffs[0], wavfilt_synth+wavfilt_len*2*n, \
																	wavfilt_synth+wavfilt_len*2*m, wavfilt_len, wavfilt_offset);
				}
				dual_tree_coeffs[0][2*n+m]->reconstruct_level(tmp[2*n+m], first_lev_synth+first_lev_len*2*n, 
															  first_lev_synth+first_lev_len*2*m, first_lev_len, first_lev_offset);
			}
		}
				
		copy_out(tmp,dst,m_w*m_h);
		
		for (int i=0; i<4; i++) {
			delete[] tmp[i];
		}
		delete[] tmp;
		
		
		
	}
	
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 


};

#endif
