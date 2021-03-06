#include "SpatialHistogramReco.h"


// here, we try more ideas from :
//
// Xiaoyang Tan and Bill Triggs
// Enhanced Local Texture Feature Sets for Face Recognition Under Difﬁcult Lighting Conditions
//
// instead of collecting uniform lbp or ltp bit features, we restrict it to the 4 bits from Clbp
// but in the very same manner, make a distanceTransform of each bitplane, concat that to the feature vector.
//
// "Description of Interest Regions with Center-Symmetric Local Binary Patterns"
// (http://www.ee.oulu.fi/mvg/files/pdf/pdf_750.pdf).
// 
// (thanks, alberto.fernandez, for correcting the link and hinting at the threshold value!)
//

class ClbpDist : public SpatialHistogramReco
{
protected:

    //! the bread-and-butter thing, collect a histogram (per patch)
    virtual void oper(const Mat & src, Mat & hist) const;

    //! choose a distance function for your algo
    virtual double distance(const Mat & hist_a, Mat & hist_b) const {
        return cv::norm(hist_a,hist_b,NORM_L2SQR); 
    }
private:
    int _numBits;
    float _scaleFac; //! (down)scale output bitplanes

public:

    ClbpDist( int gridx, int gridy, double threshold = DBL_MAX) 
        : SpatialHistogramReco(gridx,gridy,threshold,0,CV_8U,0)
        , _numBits(4+1) // added 1 center bits
        , _scaleFac(0.5f)
    {}
};



static void pattern(const Mat & src, Mat & lbp) {
    // calculate patterns
    int radius = 1;
    int t = 0; // threshold
    for(int i=radius;i<src.rows-radius;i++) 
    {
        for(int j=radius;j<src.cols-radius;j++) 
        {
            int k = 0;
            //
            // 7 0 1
            // 6 c 2
            // 5 4 3
            //
            // center and ring of neighbours:
            uchar c   = src.at<uchar>(i,j);
            uchar n[8]= {               
                src.at<uchar>(i-1,j),
                src.at<uchar>(i-1,j+1),
                src.at<uchar>(i,j+1),
                src.at<uchar>(i+1,j+1),
                src.at<uchar>(i+1,j),
                src.at<uchar>(i+1,j-1),
                src.at<uchar>(i,j-1),
                src.at<uchar>(i-1,j-1) 
            };
            // save 4 bits ( 1 for each of 4 possible diagonals )
            //  _\|/_
            //   /|\
            // this is the "central symmetric LBP" idea, from :
            // "Description of Interest Regions with Center-Symmetric Local Binary Patterns"
            // (http://www.ee.oulu.fi/mvg/files/pdf/pdf_750.pdf).
            //
            uchar & code = lbp.at<uchar>(i,j);
            code += ((n[0]-n[4])>t)<<k++;
            code += ((n[1]-n[5])>t)<<k++;
            code += ((n[2]-n[6])>t)<<k++;
            code += ((n[3]-n[7])>t)<<k++;
            // 1 bit of center value
            code += (c > ((n[0]+n[1]+n[2]+n[3]+n[4]+n[5]+n[6]+n[7])>>3))<<k++; 
        }
    }
}



void ClbpDist::oper(const Mat & src,Mat & hist) const {
    Size s = src.size();
    Mat lbp = Mat::zeros(s, CV_8U);

    pattern(src,lbp);

    // linear concatenation of all the bitplanes run through a distance transform
    //
    // resizing output dist_img by 0.5
    //
    Mat dst,dt,rz;
    for ( int i=0; i<_numBits; i++ ) 
    {
        distanceTransform( (lbp&(1<<i)), dt, DIST_L2, 3 );
        resize(dt, rz, Size(), _scaleFac,_scaleFac);
        dst.push_back(rz.reshape(0,1));
    }
    hist = dst.reshape(0,1);
}


Ptr<FaceRecognizer> createClbpDistFaceRecognizer(double threshold)
{
    return makePtr<ClbpDist>(8,8,threshold);
}
