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
#include <imagedata.h>
#include <glib/gstdio.h>
#include <safegtk.h>
#include <rtengine.h>
#include <version.h>
#include <iptcmeta.h>

#ifndef GLIBMM_EXCEPTIONS_ENABLED
#include <memory>
#endif

namespace rtengine{

const char *SnapshotInfo::kCurrentSnapshotName="####";


ImageMetaData* ImageMetaData::fromFile (const Glib::ustring& fname, const Glib::ustring &fnameMeta, const Glib::ustring &fnameMeta2, bool embed ) {

	ImageMetaData* newData = new ImageMetaData (fname,fnameMeta,fnameMeta2 );
	if( newData ){
		newData->xmpEmbedded = embed;
		if( newData->xmpEmbedded  ){
			newData->readMetadataFromImage( );
		}else{
			if( !safe_file_test(fnameMeta, Glib::FILE_TEST_EXISTS ) ){
				// Second try: search for cached file
				if( fnameMeta2.empty() || !safe_file_test(fnameMeta2, Glib::FILE_TEST_EXISTS ) ){
					newData->readMetadataFromImage( );
					newData->initRTXMP( );

					// Read old pp3 (only) next to the original file
					if( safe_file_test(fname+".pp3", Glib::FILE_TEST_EXISTS ) ){
						rtengine::procparams::ProcParams  pparams;
						int rank=0;
						if( !pparams.load( fname+".pp3",&rank ) ){
							newData->newSnapshot("PP3", pparams, false);
							newData->newSnapshot( SnapshotInfo::kCurrentSnapshotName, pparams, false);
							if( rank > 0 )
								newData->xmpData["Xmp.xmp.Rating"] = rank;

						}
					}
					return newData;
				}
			}
			newData->loadXMP( );
		}

		//xmp can be generated by other programs, so it doesn't contain RT data.
		if( newData->xmpData.findKey(Exiv2::XmpKey(Glib::ustring::compose("Xmp.rt.%1",rtengine::kXmpProcessing) )) == newData->xmpData.end() )
			newData->initRTXMP( );
	}
    return newData;
}

ImageMetaData::ImageMetaData (const Glib::ustring &fn, const Glib::ustring &fnMeta, const Glib::ustring &fnMeta2 )
   :fname(fn)
   ,fnameMeta(fnMeta)
   ,fnameMeta2(fnMeta2)
   ,exifExtracted(false)
   ,xmpEmbedded(false)
{
}

ImageMetaData::ImageMetaData ( const ImageMetaData &v  )
{
	xmpData   = v.xmpData;
	exifData  = v.exifData;
	iptcData  = v.iptcData;
	exifExtracted = v.exifExtracted;
	xmpEmbedded = v.xmpEmbedded;
	fname     = v.fname;
	fnameMeta = v.fnameMeta;
	fnameMeta2= v.fnameMeta2;
}


ImageMetaData::~ImageMetaData ()
{
}
void ImageMetaData::setOutputImageInfo(int w, int h, int bits )
{
	xmpData["Xmp.tiff.ImageWidth"] = w;
	xmpData["Xmp.tiff.ImageLength"]= h;
	//xmpData["Xmp.tiff.Compression"] = compression;
	xmpData["Xmp.tiff.SamplesPerPixel"] = 3;
	Exiv2::XmpData::iterator iter = xmpData.findKey( Exiv2::XmpKey("Xmp.tiff.BitsPerSample"));
	if( iter != xmpData.end() )
	   xmpData.erase( iter );
	xmpData["Xmp.tiff.BitsPerSample"] = bits;
	xmpData["Xmp.tiff.PhotometricInterpretation"] = 2; //2=RGB
}

void ImageMetaData::merge( bool syncExif, bool syncIPTC, bool removeProcessing )
{
	if(!exifExtracted )
		updateExif();

	xmpData["Xmp.tiff.Software"]= "RawTherapee";
	xmpData["Xmp.rt.Version"] = VERSION;
	xmpData["Xmp.tiff.Orientation"] = ePhotoOrientationNormal; // Output should be "right" orientation

	if( removeProcessing )
        for( Exiv2::XmpData::iterator iter = xmpData.begin(); iter != xmpData.end(); ){
        	if( iter->key().find( rtengine::kXmpProcessing ) !=  std::string::npos  )
        		iter = xmpData.erase(iter);
        	else
        		iter++;
        }
	if( syncIPTC )
        Exiv2::copyXmpToIptc( xmpData, iptcData );
	if( syncExif )
        Exiv2::copyXmpToExif( xmpData, exifData );

}

bool ImageMetaData::writeToImage( const Glib::ustring &fname, bool writeExif, bool writeIPTC, bool writeXmp ) const
{
	// This lock is for security with current sdk! Maybe with an update to xmp SDK 5.1 ...
	Glib::Mutex::Lock lock(* exiv2Mutex );

	Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open( fname );
	if( !image->good())
		return false;
	if( writeExif && !exifData.empty()) image->setExifData( exifData );
	if( writeIPTC && !iptcData.empty()) image->setIptcData( iptcData );
	if( writeXmp  && !xmpData.empty() ) image->setXmpData( xmpData );
	try{
	    image->writeMetadata();
	    return true;
	}catch ( Exiv2::Error e){
		return false;
	}
}

int ImageMetaData::readMetadataFromImage()
{
	// This lock is for security with current sdk! Maybe with an update to xmp SDK 5.1 ...
	Glib::Mutex::Lock lock(* exiv2Mutex );

	Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open( fname );
	image->readMetadata();
	xmpData = image->xmpData();

	Glib::ustring key;
	key = Glib::ustring::compose("Xmp.rt.%1",rtengine::kXmpMerged);
	if( xmpData.findKey(Exiv2::XmpKey( key )) == xmpData.end() ){
		// Copy embedded EXIF and IPTC into XMP
		Exiv2::copyExifToXmp(image->exifData(), xmpData);
		Exiv2::copyIptcToXmp(image->iptcData(), xmpData, 0);
		xmpData[key] = 1;
	}

	iptcData = image->iptcData();
	exifData = image->exifData();
	exifExtracted = true;

	// TODO: Lens name this need expansion:
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.aux.Lens")) == xmpData.end() ){
		Exiv2::ExifData::const_iterator iter = Exiv2::lensName( exifData );
		if(  iter != exifData.end() )
			xmpData["Xmp.aux.Lens"] =  iter->print(&exifData); /*iter->getValue()->toString();*/
	}

	if( xmpData.findKey(Exiv2::XmpKey("Xmp.aux.SerialNumber")) == xmpData.end() ){
		Exiv2::ExifData::const_iterator iter = Exiv2::serialNumber( exifData );
		if(  iter != exifData.end() )
			xmpData["Xmp.aux.SerialNumber"] = iter->getValue()->toString();
	}

	key = IPTCMeta::IPTCtags[kIPTCDate].getXmpKey();
	if( xmpData.findKey(Exiv2::XmpKey(key)) == xmpData.end() ){
		Exiv2::XmpData::const_iterator iter = xmpData.findKey( Exiv2::XmpKey("Xmp.exif.DateTimeDigitized"));
		if( iter == xmpData.end() )
			iter = xmpData.findKey( Exiv2::XmpKey("Xmp.exif.DateTimeOriginal"));
		if( iter == xmpData.end() )
			iter = xmpData.findKey( Exiv2::XmpKey("Xmp.tiff.DateTime"));
		if( iter != xmpData.end() ){
			xmpData[key] = iter->getValue()->toString();
		}
	}

    key = IPTCMeta::IPTCtags[kIPTCTitle].getXmpKey();
	if( xmpData.findKey(Exiv2::XmpKey(key)) == xmpData.end() ){
		Glib::ustring::size_type iPos = fname.find_last_of('.');
		Glib::ustring::size_type fPos = fname.find_last_of('\\');
		if( fPos == Glib::ustring::npos )
			fPos = fname.find_last_of('/');
		if( fPos == Glib::ustring::npos )
			fPos = 0;
		else
			fPos++;
		Glib::ustring title = fname.substr( fPos, iPos - fPos );
		xmpData[ key ] = title;
	}

	key = IPTCMeta::IPTCtags[kIPTCGUID].getXmpKey();
	if( xmpData.findKey(Exiv2::XmpKey(key)) == xmpData.end() ){
		Glib::ustring s1 = getCamera();
		Glib::ustring s2 = getSerialNumber();
		tm t =ImageMetaData::getDateTime ();
		Glib::ustring s3 =Glib::ustring::compose("%1%2%3%4%5%6",t.tm_year+1900,t.tm_mon+1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);

		Glib::ustring::size_type iPos = fname.find_last_of('.');
		Glib::ustring s4;
		while( --iPos >= 0 ){
			guint32 c = fname.at(iPos);
			if(  c >='0' && c <='9' )
				s4 =  Glib::ustring(1,c) + s4;
			else
				break;
		}
		Glib::ustring s = s1+s2+s3+s4;
		xmpData[ key ] = s;
	}
	//TODO add kIPTCMaxHeight, kIPTCMaxWidth from secure image size

	return 0;
}

int ImageMetaData::initRTXMP( )
{
	// write global options
    xmpData[Glib::ustring::compose("Xmp.rt.%1",rtengine::kXmpVersion)] = VERSION;

    // write empty processing list
    std::string baseKey(Glib::ustring::compose("Xmp.rt.%1",rtengine::kXmpProcessing));
    if( xmpData.findKey(Exiv2::XmpKey(baseKey )) == xmpData.end() ){
		Exiv2::XmpTextValue tv("");
		tv.setXmpArrayType(Exiv2::XmpValue::xaBag);
		xmpData.add(Exiv2::XmpKey(baseKey), &tv);
    }
    return 0;
}

int ImageMetaData::updateExif()
{
	// This lock is for security with current sdk! Maybe with an update to xmp SDK 5.1 ...
	Glib::Mutex::Lock lock(* exiv2Mutex );

	Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open( fname );
	image->readMetadata();
	exifData = image->exifData();
	iptcData = image->iptcData();
	exifExtracted = true;
}

std::vector<ExifPair> ImageMetaData::getExifData () const
{
	std::vector<ExifPair> list;
	if( ! exifExtracted ){
		return list;
	}
	for( Exiv2::ExifData::const_iterator iter = exifData.begin(); iter != exifData.end(); iter ++){
		if( iter->tagName().find("MakerNote") != std::string::npos )
			continue;
		if( iter->groupName().find("MakerNote") != std::string::npos )
			continue;
		if( iter->tagName().substr(0,2).compare("0x") == 0 ) // unnamed tags
			continue;

		ExifPair val;
		val.field = iter->tagName();
		val.group = iter->groupName();
		val.rawValue =  iter->getValue()->toString();
		val.value =  iter->print( &exifData );
		list.push_back( val );
	}
	return list;
}

const rtengine::MetadataList ImageMetaData::getIPTCData () const 
{

	rtengine::MetadataList iptcc;

    for( IPTCMeta::IPTCtagsList_t::iterator iter = IPTCMeta::IPTCtags.begin(); iter != IPTCMeta::IPTCtags.end(); iter++ ){
    	Glib::ustring exiv2Key = iter->second.getXmpKey();
    	Exiv2::XmpData::const_iterator xIter = xmpData.findKey(Exiv2::XmpKey(exiv2Key));
    	if( xIter != xmpData.end()){
    		std::vector< Glib::ustring> v;
    		if( readIPTCFromXmp( xmpData, iter->second, v ) )
    			iptcc[iter->first]= v;
    	}
    }
    return iptcc;
}

bool ImageMetaData::getIPTCDataChanged() const
{
	std::string key= Glib::ustring::compose("Xmp.rt.%1",rtengine::kXmpMerged);
	Exiv2::XmpData::const_iterator iter= xmpData.findKey( Exiv2::XmpKey(key));
	if( iter != xmpData.end() ){
		return iter->toLong()==2;
	}
	return false;
}

void ImageMetaData::setIPTCData( const rtengine::MetadataList &meta )
{

    for( rtengine::MetadataList::const_iterator iter = meta.begin(); iter != meta.end(); iter++ ){
    	std::string key = iter->first;

    	IPTCMeta::IPTCtagsList_t::iterator it = IPTCMeta::IPTCtags.begin();
    	while ( it != IPTCMeta::IPTCtags.end() && it->first.compare(key)!=0 )it++;
    	if( it == IPTCMeta::IPTCtags.end() )
    		continue;
    	IPTCMeta &metaTag = it->second;
    	//IPTCMeta &metaTag = IPTCMeta::IPTCtags[key];

    	if( metaTag.readOnly )
    		continue;

    	Glib::ustring Exiv2Key = metaTag.getXmpKey();
    	Exiv2::XmpData::iterator xIter=xmpData.findKey( Exiv2::XmpKey( Exiv2Key ) );
    	if( iter->second.empty() || iter->second[0].empty() ){ // delete key
    		if( xIter != xmpData.end() )
    			xmpData.erase( xIter );
    	}else {

    		switch( metaTag.arrType  ){ // change single value
    		case Exiv2::xmpText:
				{
					if( xIter != xmpData.end() )
						xmpData.erase( xIter );
					Exiv2::Value::AutoPtr v= Exiv2::Value::create(Exiv2::xmpText);;
					v->read( iter->second[0] );
					xmpData.add(Exiv2::XmpKey(Exiv2Key), v.get());
					break;
				}
    		case Exiv2::xmpBag:
				{
					int j=1;
					while( (xIter=xmpData.findKey( Exiv2::XmpKey( metaTag.getXmpKey(j) ) )) != xmpData.end() ){
						xmpData.erase( xIter );
						j++;
					}
					Exiv2::Value::AutoPtr v= Exiv2::Value::create(Exiv2::xmpBag);;
					for( int i=0; i< iter->second.size();i++ )
					   v->read( iter->second[i] );
					xmpData.add(Exiv2::XmpKey(Exiv2Key), v.get());
					break;
				}
    		case Exiv2::xmpSeq:
				{
					int j=1;
					while( (xIter=xmpData.findKey( Exiv2::XmpKey( metaTag.getXmpKey(j) ) )) != xmpData.end() ){
						xmpData.erase( xIter );
						j++;
					}
					Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::xmpSeq);
					for( int i=0; i< iter->second.size();i++ )
					   v->read( iter->second[i] );
					xmpData.add(Exiv2::XmpKey(Exiv2Key), v.get());
					break;
				}
    		case Exiv2::langAlt:
				{
					if( xIter != xmpData.end() )
						xmpData.erase( xIter );
					Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::langAlt);
					v->read( iter->second[0] );
					xmpData.add(Exiv2::XmpKey(Exiv2Key), v.get());
					break;
				}
    		}
    	}
    }
   	std::string key = Glib::ustring::compose("Xmp.rt.%1",rtengine::kXmpMerged);
   	xmpData[key]=2;
    saveXMP();
}

int  ImageMetaData::getRank  ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.xmp.Rating")) != xmpData.end() ){
		const Exiv2::Value &rankValue = xmpData["Xmp.xmp.Rating"].value();
		long r = rankValue.toLong();
		if( rankValue.ok() )
			return (int)r;
	}
	return 0;
}

void ImageMetaData::setRank  (int rank)
{
	if( rank>=-1 && rank<=5 ){
	   xmpData["Xmp.xmp.Rating"] = rank;
	   saveXMP();
	}
}

Glib::ustring ImageMetaData::getLabel()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.xmp.Label")) != xmpData.end() ){
		return xmpData["Xmp.xmp.Label"].value().toString();
	}
	return Glib::ustring();
}

void ImageMetaData::setLabel(const Glib::ustring &colorlabel)
{
	xmpData["Xmp.xmp.Label"] = colorlabel;
	saveXMP();
}

tm ImageMetaData::getDateTime ()
{
	tm dt;
	memset(&dt,0,sizeof(dt));
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.tiff.DateTime")) != xmpData.end() ){
		Glib::ustring s( xmpData["Xmp.tiff.DateTime"].value().toString() );
		sscanf(s.c_str(),"%d-%d-%dT%d:%d:%d",&dt.tm_year, &dt.tm_mon, &dt.tm_mday, &dt.tm_hour, &dt.tm_min, &dt.tm_sec);
		dt.tm_mon-=1;
		dt.tm_year-=1900;
		mktime(&dt);
	}
	return dt;
}

time_t ImageMetaData::getDateTimeAsTS()
{
	tm dt;
	memset(&dt,0,sizeof(dt));
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.tiff.DateTime")) != xmpData.end() ){
		Glib::ustring s( xmpData["Xmp.tiff.DateTime"].value().toString() );
		sscanf(s.c_str(),"%d-%d-%dT%d:%d:%d",&dt.tm_year, &dt.tm_mon, &dt.tm_mday, &dt.tm_hour, &dt.tm_min, &dt.tm_sec);
		dt.tm_mon-=1;
		dt.tm_year-=1900;
		return mktime(&dt);
	}
	return 0;
}

int ImageMetaData::getISOSpeed ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.exif.ISOSpeedRatings")) != xmpData.end() ){
		return xmpData["Xmp.exif.ISOSpeedRatings"].value().toLong();
	}
	return 0;
}

double ImageMetaData::getFNumber  ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.exif.FNumber")) != xmpData.end() ){
		const Exiv2::Rational r=xmpData["Xmp.exif.FNumber"].value().toRational();
		return r.second != 0 ? double(r.first)/r.second:0.;
	}
	return 0.;
}

double ImageMetaData::getFocalLen ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.exif.FocalLength")) != xmpData.end() ){
		const Exiv2::Rational r=xmpData["Xmp.exif.FocalLength"].value().toRational();
		return r.second != 0 ? double(r.first)/r.second:0.;
	}
	return 0.;
}

double ImageMetaData::getShutterSpeed ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.exif.ExposureTime")) != xmpData.end() ){
		const Exiv2::Rational r=xmpData["Xmp.exif.ExposureTime"].value().toRational();
		return r.second != 0 ? double(r.first)/r.second:0.;
	}
	return 0.;
}
double ImageMetaData::getExpComp  ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.exif.ExposureBiasValue")) != xmpData.end() ){
		const Exiv2::Rational r=xmpData["Xmp.exif.ExposureBiasValue"].value().toRational();
		return r.second != 0 ? double(r.first)/r.second:0.;
	}
	return 0.;
}

bool ImageMetaData::getFlashFired()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.exif.Flash/Fired")) != xmpData.end() ){
		return (xmpData["Xmp.exif.Flash/Fired"].value().toString().compare("True") == 0);
	}
	return false;
}

std::string ImageMetaData::getMake()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.tiff.Make")) != xmpData.end() ){
		return xmpData["Xmp.tiff.Make"].value().toString();
	}
	return "";
}

std::string ImageMetaData::getModel()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.tiff.Model")) != xmpData.end() ){
		return xmpData["Xmp.tiff.Model"].value().toString();
	}
	return "";
}

std::string ImageMetaData::getCamera()
{
	std::string make = getMake();
	std::string model = getModel();

    static const char *corp[] =
      { "Canon", "NIKON", "EPSON", "KODAK", "Kodak", "OLYMPUS", "PENTAX",
        "MINOLTA", "Minolta", "Konica", "CASIO", "Sinar", "Phase One",
        "SAMSUNG", "Mamiya", "MOTOROLA" };
    if( !make.empty()){
		for (int i = 0; i < sizeof corp / sizeof *corp; i++)
			if (make.find(corp[i]) != std::string::npos) { /* Simplify company names */
				make = corp[i];
				break;
			}
		make.erase(make.find_last_not_of(' ') + 1);
		std::size_t i = 0;
		if (make.find("KODAK") != std::string::npos) {
			if ((i = model.find(" DIGITAL CAMERA")) != std::string::npos ||
				(i = model.find(" Digital Camera")) != std::string::npos ||
				(i = model.find("FILE VERSION")))
				model.resize(i);
		}
    }
	if( !model.empty() ){
		model.erase(model.find_last_not_of(' ') + 1);

		if (!strncasecmp(model.c_str(), make.c_str(), make.size()))
			if (model.at(make.size()) == ' ')
				model.erase(0, make.size() + 1);
		if (model.find("Digital Camera ") != std::string::npos)
			model.erase(0, 15);
	}

	return make + " " + model;
}

ImageMetaData::ePhotoOrientation ImageMetaData::getOrientation ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.tiff.Model")) != xmpData.end() ){
		return ePhotoOrientation(xmpData["Xmp.tiff.Model"].value().toLong());
	}
	return ePhotoOrientationNormal; // default is normal
}

std::string ImageMetaData::getLens()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.aux.Lens")) != xmpData.end() ){
		return xmpData["Xmp.aux.Lens"].value().toString();
	}
	return "";
}

std::string ImageMetaData::getSerialNumber ()
{
	if( xmpData.findKey(Exiv2::XmpKey("Xmp.aux.SerialNumber")) != xmpData.end() ){
		return xmpData["Xmp.aux.SerialNumber"].value().toString();
	}
	return "";
}


std::string ImageMetaData::apertureToString (double aperture) {

    char buffer[256];
    sprintf (buffer, "%0.1f", aperture);
    return buffer;
}

std::string ImageMetaData::shutterToString (double shutter) {

    char buffer[256];
    if (shutter > 0.0 && shutter < 0.9)
        sprintf (buffer, "1/%0.0f", 1.0 / shutter);
    else
        sprintf (buffer, "%0.1f", shutter);
    return buffer;
}

std::string ImageMetaData::expcompToString (double expcomp, bool maskZeroexpcomp) {

    char buffer[256];
    if (maskZeroexpcomp==true){
        if (expcomp!=0.0){
    	    sprintf (buffer, "%0.2f", expcomp);
    	    return buffer;
        }
        else
    	    return "";
    }
    else{
    	sprintf (buffer, "%0.2f", expcomp);
    	return buffer;
    }
}

double ImageMetaData::shutterFromString (std::string s) {

    int i = s.find_first_of ('/');
    if (i==std::string::npos)
        return atof (s.c_str());
    else 
        return atof (s.substr(0,i).c_str()) / atof (s.substr(i+1).c_str());
}

double ImageMetaData::apertureFromString (std::string s) {

    return atof (s.c_str());
}

int ImageMetaData::loadXMP(  )
{
	// This lock is a must with current sdk! Maybe with an update to xmp SDK 5.1 ...
	Glib::Mutex::Lock lock(*exiv2Mutex );

    FILE *f = safe_g_fopen( fnameMeta,"rb");
	if ( f == NULL ){
		if( fnameMeta2.empty() )
			return 1;

		// Second try with backup
		f = safe_g_fopen( fnameMeta2,"rb");
		if ( f == NULL )
		   return 1;
	}
	fseek (f, 0, SEEK_END);
	long filesize = ftell (f);
	if( filesize <=0 )
		return 2;

	char *buffer=new char[filesize];
	fseek (f, 0, SEEK_SET);
    fread( buffer, 1, filesize, f);
    fclose(f);
    std::string xmpPacket(buffer,buffer+filesize);
    delete [] buffer;
    try{
		if (0 != Exiv2::XmpParser::decode(xmpData,xmpPacket) ) {
			return 2;
		}
    }catch(Exiv2::Error &e){
    	printf("Exception in parser: %s\n", e.what());
    	return 2;
    }

    return 0;
}

int ImageMetaData::saveXMP( ) const
{
	if( xmpEmbedded )
		return writeToImage(fname,true,true,true );

	// This lock is a must with current sdk! Maybe with an update to xmp SDK 5.1 ...
	Glib::Mutex::Lock lock(*exiv2Mutex );

    std::string xmpPacket;
    if (0 != Exiv2::XmpParser::encode(xmpPacket, xmpData)) {
        return 1;
    }

    // TODO: why there is the need to reread? It seems XmpParser::encode messes up xmpData!
    Exiv2::XmpParser::decode( *const_cast< Exiv2::XmpData *>(  &xmpData ),xmpPacket);

    FILE *f = safe_g_fopen (fnameMeta, "wt");
    if (f!=NULL){
    	 fwrite( xmpPacket.c_str(), 1, xmpPacket.size(), f );
         fclose (f);
         f=NULL;
    }
    if( !fnameMeta2.empty() ){
		f = safe_g_fopen (fnameMeta2, "wt");
		if (f!=NULL) {
			 fwrite( xmpPacket.c_str(), 1, xmpPacket.size(), f );
			 fclose (f);
			 f=NULL;
		}
    }
    return 0;
}

int ImageMetaData::resync( )
{
	Glib::Mutex::Lock lock(* exiv2Mutex );

	Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open( fname );
	image->readMetadata();
	Exiv2::XmpData xmp = image->xmpData();

	rtengine::MetadataList iptcc;
	for( IPTCMeta::IPTCtagsList_t::iterator iter = IPTCMeta::IPTCtags.begin(); iter != IPTCMeta::IPTCtags.end(); iter++ ){
		Glib::ustring exiv2Key = iter->second.getXmpKey();
		Exiv2::XmpData::const_iterator xIter = xmp.findKey(Exiv2::XmpKey(exiv2Key));
		if( xIter != xmp.end()){
			std::vector< Glib::ustring> v;
			if( readIPTCFromXmp( xmp, iter->second, v ) )
				iptcc[iter->first]= v;
		}else
			iptcc[iter->first]= std::vector< Glib::ustring>(); // delete
	}
	setIPTCData( iptcc );
	Exiv2::copyIptcToXmp(image->iptcData(), xmpData, 0);
	return saveXMP( );
}

/* Create a new snapshot with the given name and save inside it the procparams
 * Append the processing parameters to the end of the list then save the xmp file
 * Returns the id of the new snapshot
 */
int ImageMetaData::newSnapshot(const Glib::ustring &newName, const rtengine::procparams::ProcParams& params, bool queued)
{
	int currentSet = 1;
	int newId=-1;
    std::string keyID,keyName,keyParams,keyQueued;
    std::vector<int> idList;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
    	keyName   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshot);
    	keyQueued = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpQueued);
    	keyParams = Glib::ustring::compose("Xmp.rt.%1[%2]/",rtengine::kXmpProcessing,currentSet);

		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			break;
		int xmpid( iter->getValue()->toLong() );
		idList.push_back( xmpid );
		currentSet++;
	};

    do{
       newId = rand();
    }while ( !idList.empty() && std::find(idList.begin(),idList.end(),newId )!= idList.end() );

	xmpData[keyName] = newName;
	xmpData[keyID] = newId;
	if( queued )
	   xmpData[keyQueued] = queued;
	params.saveIntoXMP(xmpData, keyParams);
	if( saveXMP() )
		return -1;

	return newId;
}

/* Delete the snapshot with the given id
 * then save the xmp file
 */
bool ImageMetaData::deleteSnapshot( int id )
{
	int currentSet = 1;
    std::string keyParams,keyID,keyParamsNext,keyQ;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
    	keyParams = Glib::ustring::compose("Xmp.rt.%1[%2]",rtengine::kXmpProcessing,currentSet);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			return false;
		int xmpid( iter->getValue()->toLong() );
		if( xmpid==id){
			keyQ = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpQueued);
			if( (iter =xmpData.findKey(Exiv2::XmpKey(keyQ))) != xmpData.end() )
				if( iter->getValue()->toString().compare("True") == 0 )
					return false; // cannot delete snapshot in queue
			iter = xmpData.begin();
			while( iter != xmpData.end() ){
				if( iter->key().find( keyParams ) != std::string::npos )
					iter = xmpData.erase( iter );
				else
					iter++;
			}
			break;
		}
	    currentSet++;
	}
    /* There must be no "hole" in numeration of array items Xmp.rt.Processing[%1]
     * so we must shift down all elements to cover the position 'currentSet' deleted
     * Starting from currentSet+1, reinsert each element into new element of index currentSet
     * then currentSet+2 into currentSet+1 etc...
     */
    while( 1 ){
       	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet+1,rtengine::kXmpSnapshotId);
        keyParams = Glib::ustring::compose("Xmp.rt.%1[%2]",rtengine::kXmpProcessing,currentSet+1);
    	Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
    	if( iter == xmpData.end() )
    		break;
    	Exiv2::XmpData::iterator iter2 = xmpData.begin();
    	while( iter2 != xmpData.end() ){
    		if( iter2->key().find( keyParams ) != std::string::npos ){
    			Glib::ustring keyNew( Glib::ustring::compose("Xmp.rt.%1[%2]%3",rtengine::kXmpProcessing,currentSet,iter2->key().substr( keyParams.size() )) );
    			xmpData[keyNew] = iter2->getValue()->toString();
    			iter2 = xmpData.erase( iter2 );
    		}else
    			iter2++;
    	}
    	currentSet++;
    }

    /*for( Exiv2::XmpData::iterator iter = xmpData.begin(); iter != xmpData.end();iter++){
    	printf("%s = %s\n",iter->key().c_str(),iter->value().toString().c_str());
    }*/

	return !saveXMP();
}

bool ImageMetaData::deleteAllSnapshots()
{
	std::string keyParams = Glib::ustring::compose("Xmp.rt.%1",rtengine::kXmpProcessing );

    for(Exiv2::XmpData::iterator iter = xmpData.begin(); iter != xmpData.end(); ){
    	if( iter->key().find( keyParams ) != std::string::npos ){
    		iter = xmpData.erase( iter );
    	}else
    		iter ++;
    }
    Exiv2::XmpTextValue tv("");
    tv.setXmpArrayType(Exiv2::XmpValue::xaBag);
    xmpData.add(Exiv2::XmpKey(keyParams), &tv);
    return !saveXMP();
}

/* Rename the snapshot with the given id
 * then save the xmp file
 */
bool ImageMetaData::renameSnapshot(int id, const Glib::ustring &newname )
{
    int currentSet = 1;
    std::string keyName,keyID;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
    	keyName   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshot);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			return false;
		int xmpid( iter->getValue()->toLong() );

		if( xmpid == id){
			xmpData[keyName] = newname;
			break;
		}
		currentSet++;
    }
    return !saveXMP();
}

/*
 *  Read all the snapshots from the xmpData
 */
snapshotsList_t ImageMetaData::getSnapshotsList()
{
	snapshotsList_t list;
    int currentSet=1;
    std::string key,keyParams,keyID;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
    	keyParams = Glib::ustring::compose("Xmp.rt.%1[%2]/",rtengine::kXmpProcessing,currentSet);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			break;
		int id( xmpData[keyID].value().toLong() );
		SnapshotInfo snapshotInfo;

		int r = snapshotInfo.params.loadFromXMP( xmpData, keyParams);
		if( !r ){
			snapshotInfo.id = id;
	    	key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshot);
	    	if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
	    		snapshotInfo.name = xmpData[key].value().toString();
	    	key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpOutput);
	    	if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
	    		snapshotInfo.outputFilename = xmpData[key].value().toString();
	    	key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpQueued);
	    	if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
	    		snapshotInfo.queued = (xmpData[key].value().toString().compare("True") == 0);
	    	key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSaved);
	    	if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
	    		snapshotInfo.saved = (xmpData[key].value().toString().compare("True") == 0);
			list[id] = snapshotInfo;
		}
		currentSet ++;
    }
    return list;
}

SnapshotInfo ImageMetaData::getSnapshot( int id )
{
	SnapshotInfo snapshotInfo;
	snapshotInfo.id = -1;
    int currentSet=1;
    std::string key,keyParams,keyID;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
    	keyParams = Glib::ustring::compose("Xmp.rt.%1[%2]/",rtengine::kXmpProcessing,currentSet);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			break;
		int xmpid( xmpData[keyID].value().toLong() );
		if( xmpid == id ){
			int r = snapshotInfo.params.loadFromXMP( xmpData, keyParams);
			if( !r ){
				snapshotInfo.id = xmpid;
				key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshot);
				if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
					snapshotInfo.name = xmpData[key].value().toString();
				key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpOutput);
				if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
					snapshotInfo.outputFilename = xmpData[key].value().toString();
				key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpQueued);
				if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
					snapshotInfo.queued = (xmpData[key].value().toString().compare("True") == 0);
				key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSaved);
				if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() )
					snapshotInfo.saved = (xmpData[key].value().toString().compare("True") == 0);
				break;
			}
		}
		currentSet ++;
    }
    return snapshotInfo;
}

int ImageMetaData::getSnapshotId( const Glib::ustring &snapshotName )
{
    int found=-1;
	int currentSet=1;
	std::string keyId,keyName,keyParams;
	do{
		keyId     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
		keyName   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshot);
		keyParams = Glib::ustring::compose("Xmp.rt.%1[%2]/",rtengine::kXmpProcessing,currentSet);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyId));
		if( iter == xmpData.end() )
		  break;
		int xmpid( iter->getValue()->toLong());

		Glib::ustring name( xmpData[keyName].value().toString() );
		if( name.compare( snapshotName )==0 ){
		   found = xmpid;
		   break;
		}
		currentSet ++;
	}while( found<0 );
	return found;
}

bool ImageMetaData::updateSnapshot( int id, const rtengine::procparams::ProcParams& params)
{
    int found=-1;
	int currentSet=1;
	std::string keyId,keyName,keyParams;
	do{
		keyId     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
		keyName   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshot);
		keyParams = Glib::ustring::compose("Xmp.rt.%1[%2]/",rtengine::kXmpProcessing,currentSet);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyId));
		if( iter == xmpData.end() )
		  break;
		int xmpid( iter->getValue()->toLong());

		if( xmpid==id  ){
		   params.saveIntoXMP( xmpData, keyParams );
		   return !saveXMP();
		}
		currentSet ++;
	}while( found<0 );
	return false;
}

int ImageMetaData::getNumQueued()
{
	int count =0;
	int currentSet=1;
    std::string key,keyID;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);

		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			break;
		int id( xmpData[keyID].value().toLong() );

    	key   = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpQueued);
    	if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() ){
    		if( (xmpData[key].value().toString().compare("True") == 0) )
    			count++;
    	}
    	currentSet ++;
    }
    return count;
}

bool  ImageMetaData::setQueuedSnapshot( int id, bool inqueue )
{
    int currentSet=1;
    std::string key,keyID;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			break;
		int xmpid( xmpData[keyID].value().toLong() );
		if( xmpid == id ){
			bool oldvalue = false;
			key = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpQueued);
			if( xmpData.findKey(Exiv2::XmpKey(key))!= xmpData.end() && (xmpData[key].value().toString().compare("True")==0) )
				oldvalue = true;

			if( inqueue != oldvalue ){
				xmpData[key]= inqueue;
				saveXMP();
			}
			return true;
		}
		currentSet ++;
    }
    return false;
}

bool ImageMetaData::setSavedSnapshot( int id, bool saved, const Glib::ustring &filename )
{
    int currentSet=1;
    std::string key,keyID;
    while( 1 ){
    	keyID     = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			break;
		int xmpid( xmpData[keyID].value().toLong() );
		if( xmpid == id ){
			key = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSaved);
			xmpData[key]= saved;
			if( saved ){
				key = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpQueued);
				xmpData[key]= false;
			}
			key = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpOutput);
			xmpData[key]= filename;
			return !saveXMP();
		}
		currentSet ++;
    }
    return false;
}

int ImageMetaData::getSavedSnapshots( )
{
	int saved=0;
    int currentSet=1;
    std::string key,keyID;
    while( 1 ){
    	keyID = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSnapshotId);
		key = Glib::ustring::compose("Xmp.rt.%1[%2]/rt:%3",rtengine::kXmpProcessing,currentSet,rtengine::kXmpSaved);
		Exiv2::XmpData::iterator iter = xmpData.findKey(Exiv2::XmpKey(keyID));
		if( iter == xmpData.end() )
			break;
		iter = xmpData.findKey(Exiv2::XmpKey(key));
		if( iter != xmpData.end() && (iter->getValue()->toString().compare("True")==0) )
			saved++;
		currentSet ++;
    }
    return saved;
}

}
