#include<unistd.h>
#include<stdio.h>
#include<opencv2/opencv.hpp>
#include<opencv2/imgproc.hpp>
#include<math.h>

using namespace cv;
using namespace std;

//直線のクラス
class MyLines
{
    public:
    float rho;
    float theta;
    float x0;
    float y0;
    float my;
    float mx;
    float cx;
    float cy;

    MyLines(float r,float t)
    {
        rho = r;
        theta = t;
        float cosT = cos(theta);
        float sinT = sin(theta);

        //直線上の点の計算方法
        // y = my * x + cy
        // x = mx * y * cx
        my = -(cosT / sinT);
        cy = rho / sinT;
        mx = -(sinT / cosT);
        cx = cosT * rho;

        //直線上の点
        x0 = cosT * rho;
        y0 = sinT * rho;
    }
};

float midx1 = 0; //左右線の中点のX座標(Y座標:180)
float midx2 = 0; //左右線の中点のX座標(Y座標:240)
float midx3 = 0; //左右線の中点のX座標(Y座標:300)

int main(int argc, char **argv)
{
    int i;
    float *line, rho, theta;
    double cosT, sinT, x0, y0;
    CvMemStorage *storage;
    CvSeq *lines = 0;
    CvPoint *point, pt1, pt2;

    //カメラから画像を取得
    CvCapture *videoCapture = cvCreateCameraCapture(0);

    IplImage *image;
    IplImage * gray;

    //画像サイズを取得
    image = cvQueryFrame(videoCapture);
    CvSize frameSize = cvGetSize(image);

    //画面X座標中心、Y座標が180,240,300pix
    float widthRate = frameSize.width * 0.05;
    float midLine = frameSize.width * 0.5;
    float y1 = frameSize.height * 0.375;
    float y2 = frameSize.height * 0.5;
    float y3 = frameSize.height * 0.625;

    while(1)
    {
        image = cvQueryFrame(videoCapture);
        gray = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);

        //下限値の値が大きいほど、より白いものが抽出される
        cvInRangeS(image, cvScalar(150, 150, 150), cvScalar(255, 255, 255), gray);

        //ハフ変換のための前処理
        cvCanny(gray, gray, 50, 200, 3);
        storage = cvCreateMemStorage(0);

        //ハフ変換による線の検出
        lines = cvHoughLines2(gray, storage, CV_HOUGH_STANDARD, 1, CV_PI / 180, 50, 0, 0);
        vector<MyLines> lineList;
        for(i = 0; i < MIN(lines->total, 100); i++)
        {
            line = (float*)cvGetSeqElem(lines, i);
            //rhoは原点から直線までの距離、thetaは直線の角度
            rho = line[0];
            theta = line[1];

            //直線上の1点を算出
            cosT = cos(theta);
            sinT = sin(theta);
            x0 = cosT * rho;
            y0 = sinT * rho;

            bool isOneLine = false;
            for(int i = 0; i<lineList.size(); ++i)
            {
                //位置と角度が一定範囲内は同じ線と見なす
                if((fabs(lineList[i].y0 - y0) < widthRate) && (fabs(lineList[i].theta - theta) < 0.3))
                {
                    isOneLine = true;
                    break;
                }
            }

            pt1.x = cvRound(x0 + 1000 * (-sinT));
            pt1.y = cvRound(y0 + 1000 * (cosT));
            pt2.x = cvRound(x0 - 1000 * (-sinT));
            pt2.y = cvRound(y0 - 1000 * (cosT));

            //異なる線のみ、リストに入れる(横線は対象外)
        	fprintf(stderr,"|--theta:%.03f\n",theta);
            if((!isOneLine) && (!((theta > 0.3) && (theta < 2.8))))
            {
                cvLine(image, pt1, pt2, cvScalar(255, 255, 0), 1, 1, 0);
                lineList.push_back(MyLines(rho, theta));
            }
        }

        //画面X中線から左側最も近い線と右側最も近い線を取得
        MyLines left(0, 0), right(frameSize.width, 0);
        for(int i = 0; i<lineList.size(); ++i)
        {
            //fprintf(stderr,"|--rho:%.03f theta:%.03f x:%.03f y:%.03f\n",lineList[i].rho,lineList[i].theta,
            //        lineList[i].x0,lineList[i].y0);
            if((lineList[i].x0 <= midLine) && (lineList[i].x0 > left.x0))
            {
                left = lineList[i];
            }
            else if((lineList[i].x0 > midLine) && (lineList[i].x0 < right.x0))
            {
                right = lineList[i];
            }
            else
            {
                //何もしない
            }
        }

        //fprintf(stderr,"left: %f %f right:%f %f\n",left.rho,left.theta,right.rho,right.theta);
        cosT = cos(left.theta);
        sinT = sin(left.theta);
        pt1.x = cvRound(left.x0 + 1000 * (-sinT));
        pt1.y = cvRound(left.y0 + 1000 * (cosT));
        pt2.x = cvRound(left.x0 - 1000 * (-sinT));
        pt2.y = cvRound(left.y0 - 1000 * (cosT));
        cvLine(image, pt1, pt2, cvScalar(255, 0, 0), 3, 8, 0);
        cosT = cos(right.theta);
        sinT = sin(right.theta);
        pt1.x = cvRound(right.x0 + 1000 * (-sinT));
        pt1.y = cvRound(right.y0 + 1000 * (cosT));
        pt2.x = cvRound(right.x0 - 1000 * (-sinT));
        pt2.y = cvRound(right.y0 - 1000 * (cosT));
        cvLine(image, pt1, pt2, cvScalar(255, 0, 0), 3, 8, 0);

        //左線と右線のY座標が180,240,300pixでのX座標を算出
        float lx1 = left.mx * y1 + left.cx;
        float lx2 = left.mx * y2 + left.cx;
        float lx3 = left.mx * y3 + left.cx;
        float rx1 = right.mx * y1 + right.cx;
        float rx2 = right.mx * y2 + right.cx;
        float rx3 = right.mx * y3 + right.cx;

        //Y座標が180,240,300pixでの左右線の中点のX座標を算出
        midx1 = (rx1+lx1) / 2;
        midx2 = (rx2+lx2) / 2;
        midx3 = (rx3+lx3) / 2;

        cvCircle(image, cvPoint(lx1,y1), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(lx2,y2), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(lx3,y3), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(rx1,y1), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(rx2,y2), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(rx3,y3), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(midx1, y1), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(midx2, y2), 5, cvScalar(0, 0, 255), 5, 0);
        cvCircle(image, cvPoint(midx3, y3), 5, cvScalar(0, 0, 255), 5, 0);

        //検出結果表示用のウィンドウを確保し表示する
        cvNamedWindow("Hough_line_standard", CV_WINDOW_AUTOSIZE);
        cvShowImage("Hough_line_standard", image);
        int ret = cvWaitKey(100);
        if(ret > 0)
        break;
    }

    cvDestroyWindow("Hough_line_standard");
    cvDestroyWindow("Hough_line_standard");
    cvReleaseImage(&image);
    cvReleaseImage(&gray);
    cvReleaseMemStorage(&storage);

    return 0;
}
