// RecordVideo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <vector>

#include "CameraApi.h"
#include "CameraDefine.H"
#include "CameraGrabber.h"
#include "CameraImage.h"
#include "CameraStatus.h"
#include "SerialDll.h"
using namespace std;

INT	cameraNum = 2;
tSdkCameraDevInfo	cameraInfo[2];
int VideoLenth = 200;

struct ImageData
{
	BYTE *pFrameBuffer;
	tSdkFrameHead pFrameHead;
};
class Camera
{
public:
	CameraHandle m_hCamera;
	tSdkCameraCapbility m_sCameraInfo;
	BYTE* FrameBufferRGB24;
	vector<ImageData> m_Buffer;
	int m_BufIdx = 0;
	bool m_isRecord = false;

	static void WINAPI CameraCallBackOne(CameraHandle hCamera, BYTE *pFrameBuffer, tSdkFrameHead* pFrameHead, PVOID pContext)
	{
		((Camera*)pContext)->onImageData(pFrameBuffer, pFrameHead);
	}
	static void WINAPI CameraCallBackTwo(CameraHandle hCamera, BYTE *pFrameBuffer, tSdkFrameHead* pFrameHead, PVOID pContext)
	{
		((Camera*)pContext)->onImageData(pFrameBuffer, pFrameHead);
	}
public:
	Camera()
	{
	}
	~Camera()
	{
	}
	void init(int index)
	{
		int ErrorID = 0;
		if (0 == index)
		{
			ErrorID = CameraInit(&cameraInfo[0], -1, -1, &m_hCamera);
			if (!ErrorID)
				printf("CameraInit\n");
			else
				printf("CameraInitError %d\n", ErrorID);
			ErrorID = CameraSetCallbackFunction(m_hCamera, CameraCallBackOne, (PVOID)this, NULL);
			if (!ErrorID)
				printf("CameraSetCallbackFunction\n");
			else
				printf("CameraSetCallbackFunctionError %d\n", ErrorID);
		}
		else
		{
			ErrorID = CameraInit(&cameraInfo[1], -1, -1, &m_hCamera);
			if (!ErrorID)
				printf("CameraInit\n");
			else
				printf("CameraInitError %d\n", ErrorID);
			ErrorID = CameraSetCallbackFunction(m_hCamera, CameraCallBackTwo, (PVOID)this, NULL);
			if (!ErrorID)
				printf("CameraSetCallbackFunction\n");
			else
				printf("CameraSetCallbackFunctionError %d\n", ErrorID);
		}

		ErrorID = CameraGetCapability(m_hCamera, &m_sCameraInfo);
		if (!ErrorID)
			printf("CameraGetCapability\n");
		else
			printf("CameraGetCapabilityError %d\n", ErrorID);
		FrameBufferRGB24 = CameraAlignMalloc(
			m_sCameraInfo.sResolutionRange.iWidthMax*m_sCameraInfo.sResolutionRange.iHeightMax * 3,
			16);
		ErrorID = CameraSetAeState(m_hCamera, false);
		if (!ErrorID)
			printf("CameraSetAeState\n");
		else
			printf("CameraSetAeStateError %d\n", ErrorID);
		ErrorID = CameraPlay(m_hCamera);
		if (!ErrorID)
			printf("CameraPlay\n");
		else
			printf("CameraPlayError %d\n", ErrorID);
		printf("相机%d初始化完成\n", m_hCamera);

		for (size_t i = 0; i < VideoLenth; i++)
		{
			ImageData data;
			data.pFrameBuffer = CameraAlignMalloc(
				m_sCameraInfo.sResolutionRange.iWidthMax*m_sCameraInfo.sResolutionRange.iHeightMax * 3,
				16);
			m_Buffer.push_back(data);
		}
	}

	void onImageData(BYTE *pFrameBuffer, tSdkFrameHead* pFrameHead)
	{
		if (m_isRecord && 0 == CameraImageProcess(m_hCamera, pFrameBuffer, FrameBufferRGB24, pFrameHead))
		{
			swap(FrameBufferRGB24, m_Buffer[m_BufIdx].pFrameBuffer);
			m_Buffer[m_BufIdx].pFrameHead = *pFrameHead;
			m_BufIdx = m_BufIdx + 1 < VideoLenth ? m_BufIdx + 1 : 0;
		}
	}
	void saveVideo()
	{
		int ErrorID = 0;
		char name[64];
		sprintf_s(name, "C:/CameraRecord/sucai%d.avi", m_hCamera);
		ErrorID = CameraInitRecord(m_hCamera, 1, name, true, 80, 60);
		if (!ErrorID)
			printf("Camera %d CameraInitRecord\n", m_hCamera);
		else
			printf("Camera %d CameraInitRecordError %d\n", m_hCamera, ErrorID);
		int i = m_BufIdx;
		do
		{
			CameraPushFrame(m_hCamera,m_Buffer[i].pFrameBuffer,&m_Buffer[i].pFrameHead);
			i = i + 1 < VideoLenth ? i + 1 : 0;
		} while (i != m_BufIdx);
		ErrorID = CameraStopRecord(m_hCamera);
		if (!ErrorID)
			printf("Camera %d CameraStopRecord\n", m_hCamera);
		else
			printf("Camera %d CameraStopRecordError %d\n", m_hCamera, ErrorID);
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	CameraSdkInit(1);
	CameraEnumerateDevice(cameraInfo, &cameraNum);
	printf("CameraNum = %d\n", cameraNum);
	int length = 0;
	char* com_num;
	get_com_port(&com_num, length);
	if (open_com_port(com_num))
	{
		printf("打开小盒子\n");
	}
	int Threshold = 800;
	printf("请输入声音感应灵敏度：\n");
	scanf_s("%d", &Threshold);
	set_threshold(Threshold);
	printf("请输入视频帧数：\n");
	scanf_s("%d", &VideoLenth);

	Camera* pCamera1 = new Camera();
	Camera* pCamera2 = new Camera();
	pCamera1->init(0);
	pCamera2->init(1);

	bool isVideoOK = false;
	int cueMsg = 0;
	while (true)
	{
		int msg = fetch_msg();
		if (isVideoOK || msg == 0||cueMsg == msg)
		{
			continue;
		}
		printf("消息：%d\n", msg);
		cueMsg = msg;
		switch (cueMsg)
		{
		case 2:
		{
			printf("开始录制\n");
			pCamera1->m_isRecord = true;
			pCamera2->m_isRecord = true;
		}
		break;
		case 3:
		{
			isVideoOK = true;
			Sleep(800);
			pCamera1->m_isRecord = false;
			pCamera2->m_isRecord = false;
			pCamera1->saveVideo();
			pCamera2->saveVideo();
			isVideoOK = false;
			printf("录制成功\n");
		}
		break;
		case 4:
		{
			pCamera1->m_isRecord = false;
			pCamera2->m_isRecord = false;
			printf("录制失败\n");
		}
		break;
		}
	}

	return 0;
}