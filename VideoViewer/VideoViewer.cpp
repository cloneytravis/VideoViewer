// VideoViewer.cpp : Defines the entry point for the console application.
//
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/video.hpp>
#include <cameralibrary.h>
#include "VideoViewer.h"
#include <time.h>
#include <chrono>

using CameraLibrary::CameraManager;
using CameraLibrary::CameraList;
using CameraLibrary::Camera;
using CameraLibrary::cModuleSync;
using CameraLibrary::FrameGroup;
using CameraLibrary::Frame;
using cv::Mat;
using cv::Size;
using cv::ORB;
using cv::Canny;
using cv::FONT_HERSHEY_SIMPLEX;
using cv::Feature2D;
using cv::imshow;
using cv::imread;
using cv::Scalar;
using cv::cvtColor;
using cv::createTrackbar;
using cv::namedWindow;
using cv::circle;
using cv::waitKey;
using cv::FastFeatureDetector;
using cv::copyMakeBorder;
using cv::BORDER_CONSTANT;
using cv::findContours;
using cv::RETR_TREE;
using cv::CHAIN_APPROX_SIMPLE;
using cv::HoughCircles;
using cv::Rect;
using cv::Point;
using cv::Vec4i;
using cv::putText;
using cv::Vec3f;
using cv::Point2f;
using cv::Moments;
using cv::RNG;
using cv::SimpleBlobDetector;
using cv::KeyPoint;
using cv::DetectionBasedTracker;
using cv::drawKeypoints;
using cv::DrawMatchesFlags;
using cv::COLOR_BGR2HSV;
using cv::Ptr;
using cv::TrackerBoosting;
using cv::VideoCapture;
using cv::Rect2d;
using cv::TrackerMIL;
using cv::TrackerMedianFlow;
using cv::Tracker;
using cv::TrackerKCF;
using cv::TrackerGOTURN;
using cv::inRange;
using cv::TrackerTLD;
using cv::TrackerMOSSE;
using cv::TrackerCSRT;
using cv::getTickCount;
using cv::getTickFrequency;
using cv::imwrite;
using cv::erode;
using cv::dilate;
using cv::threshold;
using cv::destroyAllWindows;
using cv::GaussianBlur;
using std::cout;
using std::ostringstream;
using std::endl;
using std::dec;
using std::string;
using std::vector;

#define CURRENTTIME std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()
int TIMEOUTTIME = 20;

//Camera Setting Variables (default settings)
Core::eVideoMode VIDEOTYPE = Core::MJPEGMode;
int EXPOSURE = 400;
const int MAX_EXPOSURE = 1300;
bool AUTO_EXPOSURE = true;
int visSpec;
int FRAME_RATE = 120;
int LED;
Camera *camOne;
Camera *camTwo;
int IMAGE_WIDTH = 640;
int IMAGE_HEIGHT = 512;


void CameraSettings(Camera *cam)
{
	cam->SetVideoType(VIDEOTYPE);
	cam->SetExposure(EXPOSURE);
	cam->SetAEC(AUTO_EXPOSURE);
	if (LED == true){
		cam->SetIntensity(0);
	}
	else {
		cam->SetIntensity(15);
	}

	if (visSpec == true) {
		cam->SetIRFilter(false);
	}
	else if (visSpec == false) {
		cam->SetIRFilter(true);
	}
	cam->SetFrameRate(FRAME_RATE);
}



/// <name>exposureCallback</name>
/// <summary>Callback to adjust the exposure of the cameras</summary>
void exposureCallback(int exp, void * userdata) {

	camOne->SetExposure(exp);
	camTwo->SetExposure(exp);

}

/// <name>connectCameras</name>
/// <summary>Checks to see if cameras connected properly</summary>
bool connectCameras()
{
	cout << "Attempting" << endl;
	bool doneInit = false;
	__int64 start = CURRENTTIME;

	//Wait until both cameras are reporting as initialized
	while (!doneInit) {
		CameraLibrary::CameraList list;

		for (int i = 0; i < list.Count(); i++) {
			if (list[i].State() == CameraLibrary::Initialized) {
				doneInit = true;
			}
		}

		for (int i = 0; i < list.Count(); i++) {
			if (list[i].State() == CameraLibrary::Initializing) {
				doneInit = false;
			}
		}

		//Check for a timeout
		__int64 now = CURRENTTIME;

		//Report a timeout
		if ((now - start) > TIMEOUTTIME) {
			cout << "timed out" << endl;
			return false;
		}
	}

	return true;
}

/// <name>main</name>
/// <summary>Starts the cameras and allows for exposure control via trackbar</summary>
int main(int argc, char *argv[]) 

{
	if (!connectCameras() == true) {
		return -1;
	}

	if (argc != 5) {
		return -1;
	}

	CameraManager::X().WaitForInitialization();
	if (!CameraManager::X().AreCamerasInitialized()) {
		return -1;
	}

	CameraList list;
	camOne = CameraManager::X().GetCamera(list[0].UID());

	if (camOne->CameraID() == 1) {
		camOne = CameraManager::X().GetCamera(list[1].UID());
		camTwo = CameraManager::X().GetCamera(list[0].UID());
	}
	else {
		camTwo = CameraManager::X().GetCamera(list[1].UID());
	}
	int camNumber = list.Count();

	if (camNumber < 2) {
		return false;
	}

	IMAGE_HEIGHT = atoi(argv[1]);
	IMAGE_WIDTH = atoi(argv[2]);
	visSpec = atoi(argv[3]);
	LED = atoi(argv[4]);

	camOne->SetVideoType(VIDEOTYPE);
	camTwo->SetVideoType(VIDEOTYPE);
	camOne->SetExposure(EXPOSURE);
	camTwo->SetExposure(EXPOSURE);
	if (LED == false) {
		camOne->SetIntensity(0);
		camTwo->SetIntensity(0);
	}
	else {
		camOne->SetIntensity(15);
		camTwo->SetIntensity(15);
	}
	if (visSpec == false) {
		camOne->SetIRFilter(false);
		camTwo->SetIRFilter(false);
	}
	else if (visSpec == true) {
		camOne->SetIRFilter(true);
		camTwo->SetIRFilter(true);
	}
	camOne->SetFrameRate(FRAME_RATE);
	camTwo->SetFrameRate(FRAME_RATE);


	cModuleSync * sync = cModuleSync::Create();
	sync->AddCamera(camOne);
	sync->AddCamera(camTwo);

	camOne->Start();
	camTwo->Start();

	namedWindow("Exp. Control", CV_WINDOW_AUTOSIZE);
	createTrackbar("Exposure", "Exp. Control", &EXPOSURE, MAX_EXPOSURE, exposureCallback);

	Mat img1(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1);
	Mat img2(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1);
	while (true) {
		FrameGroup *frameGroup = sync->GetFrameGroup();
		if (frameGroup) {
			if (sync->LastFrameGroupMode() == FrameGroup::Hardware) {
				Frame *frameOne = frameGroup->GetFrame(0);
				Frame *frameTwo = frameGroup->GetFrame(1);

				if (frameOne) {
					frameOne->Rasterize(IMAGE_WIDTH, IMAGE_HEIGHT, img1.step, 8, img1.data);
					frameOne->Release();
				}
				if (frameTwo) {
					frameTwo->Rasterize(IMAGE_WIDTH, IMAGE_HEIGHT, img2.step, 8, img2.data);
					frameTwo->Release();
				}
				frameGroup->Release();
				if (frameOne && frameTwo) {
					imshow("FrameOne", img1);
					imshow("FrameTwo", img2);

					char key = waitKey(1);
					if (key == 'q') {
						return EXPOSURE;
						break;
					}
				}
			}
		}
	}

	destroyAllWindows();
	sync->RemoveAllCameras();
	cModuleSync::Destroy(sync);
	CameraManager::X().Shutdown();



	return 1;

}

