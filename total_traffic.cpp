#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

int main(void)
{
	// VideoCapture capture("../test_1280.mp4");
	VideoCapture capture(1);
	if (!capture.isOpened()) {
		std::cerr << "Could not open camera" << std::endl;
		return 0;
	}

	int k = 2;
	int max_goodmatch;
	int final_goodmatch;
	int roi_ok=0;
	int traffic_ok=0;
	int flag = 0;
	int roi_off_ok=0;
	float ratio=0;
	int flag2=1;
	int result_traffic=0;

	int output_traffic[8]={-1,0,1,2,3,4,5,6};
	//setting
	Mat frame;
	Mat frame_gray;
  Mat resized_frame_gray;
	Mat hsv_frame;
	Mat binary_frame_red;
	Mat binary_frame_red1;
	Mat binary_frame_red2;
	Mat binary_frame_blue;
	Mat binary_frame_merge;
	Mat morphological_frame;
	Mat resized_frame;
	Mat wanted_frame;
	Mat draw_frame;

	// red value range
	Scalar lowerb_red1(0, 140, 0);
	Scalar upperb_red1(15, 255, 255);
	Scalar lowerb_red2(175, 140, 0);
	Scalar upperb_red2(179, 255, 255);
	// blue value range
	Scalar lowerb_blue(100, 100, 0);
	Scalar upperb_blue(130, 255, 255);

	//0.횡단보도 1.협로구간 2.동적장애물 3.정적장애물 4.곡선코스 5.U턴 6.자동주차
	Mat img[7];

	img[0] = imread("source/img_0.JPG", IMREAD_GRAYSCALE);
	img[1] = imread("source/img_1.JPG", IMREAD_GRAYSCALE);
	img[2] = imread("source/img_2.JPG", IMREAD_GRAYSCALE);
	img[3] = imread("source/img_3.JPG", IMREAD_GRAYSCALE);
	img[4] = imread("source/img_4.JPG", IMREAD_GRAYSCALE);
	img[5] = imread("source/img_5.JPG", IMREAD_GRAYSCALE);
	img[6] = imread("source/img_6.JPG", IMREAD_GRAYSCALE);
	int empty_test=0;

	for (int n = 0; n < 7; n++) { empty_test = empty_test || img[n].empty(); }
	if (empty_test) return -1;

	while (true)
	{
		try {
			capture >> frame; // get a new frame from webcam
			resize( frame, frame, Size( 640, 480 ), 0, 0, CV_INTER_CUBIC );
		}
		catch (Exception& e) {
			std::cerr << "Exception occurred. Ignoring frame... " << e.err
				<< std::endl;
		}

		cvtColor(frame, hsv_frame, COLOR_BGR2HSV);
		cvtColor(frame, frame_gray, COLOR_BGR2GRAY);

		inRange(hsv_frame, lowerb_red1, upperb_red1, binary_frame_red1);
		inRange(hsv_frame, lowerb_red2, upperb_red2, binary_frame_red2);
		inRange(hsv_frame, lowerb_blue, upperb_blue, binary_frame_blue);

		binary_frame_red = binary_frame_red1 | binary_frame_red2;
		binary_frame_merge = binary_frame_red | binary_frame_blue;

		//morphological opening 작은 점들을 제거
		erode(binary_frame_merge, morphological_frame, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) );
		dilate( morphological_frame, morphological_frame, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) );

		//morphological closing 영역의 구멍 메우기
		dilate( morphological_frame, morphological_frame, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) );
		erode(morphological_frame, morphological_frame, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) );

		//라벨링
		Mat img_labels,stats, centroids;
		int numOfLables = connectedComponentsWithStats(morphological_frame, img_labels, stats, centroids, 8,CV_32S);

		//영역박스 그리기
		int max = -1, idx=0;
		for (int j = 1; j < numOfLables; j++) {
				int area = stats.at<int>(j, CC_STAT_AREA);
				if ( max < area )
				{
						max = area;
						idx = j;
				}
		}

		int left = stats.at<int>(idx, CC_STAT_LEFT);
		int top  = stats.at<int>(idx, CC_STAT_TOP);
		int width = stats.at<int>(idx, CC_STAT_WIDTH);
		int height  = stats.at<int>(idx, CC_STAT_HEIGHT);

		rectangle( frame, Point(left,top), Point(left+width,top+height), Scalar(0,0,255),1 );

		ratio =(float)width/height;

		if(ratio>1.2||ratio<0.8){
			width=1;
			height=1;
		}
		else{
			if(width >100) width = 1;
			if(height >100)	height = 1;
		}

		max_goodmatch = 0;
		final_goodmatch = -1;

		if(width>50 && height>50){
			Rect rect(Point(left,top), Point(left+width,top+height));

			wanted_frame = frame(rect);

			resize( wanted_frame, resized_frame, Size( 190, 190 ), 0, 0, CV_INTER_CUBIC );
			cvtColor(resized_frame, resized_frame_gray, CV_BGR2GRAY);

			roi_ok=roi_ok+1;
			if(roi_ok>40){
				if(flag2){
					traffic_ok=traffic_ok+1;
					if(traffic_ok>7) traffic_ok=7;
					flag2=0;
					result_traffic= traffic_ok;
				}
				flag=1;
			}
			else{
				flag2=1;
			}
			imshow("output_frame",resized_frame);
		}
		else{
			roi_ok=0;
			if(flag){
				roi_off_ok=roi_off_ok+1;
				if(roi_off_ok>50){
					result_traffic=0;
					flag=0;
					roi_off_ok=0;
				}
			}
		}

		imshow("original",frame);
		imshow("morphological_frame",morphological_frame);
		// final_goodmatch //roi_ok 출력

		cout << "width-height: " << width << " , "<< height << endl;
		cout << "roi: " << roi_ok << "   tra: " << traffic_ok <<"   flag: " << flag <<  "roff: " << roi_off_ok << " ratio:" << ratio << endl;
		cout << "flag2: " << flag2 << " result_traffic: " << result_traffic << endl;
		cout << "output: " << output_traffic[result_traffic] << " traffic_ok:" << traffic_ok << endl;
		if (waitKey(10) >= 0) break;
		}
		return 0;
}
