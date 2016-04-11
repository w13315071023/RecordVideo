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
		if (0 == index)
		{
			CameraInit(&cameraInfo[0], -1, -1, &m_hCamera);
			CameraSetCallbackFunction(m_hCamera, CameraCallBackOne, (PVOID)this, NULL);
			
		}
		else
		{
			CameraInit(&cameraInfo[1], -1, -1, &m_hCamera);
			CameraSetCallbackFunction(m_hCamera, CameraCallBackTwo, (PVOID)this, NULL);
		}

		FrameBufferRGB24 = CameraAlignMalloc(
			m_sCameraInfo.sResolutionRange.iWidthMax*m_sCameraInfo.sResolutionRange.iHeightMax * 3,
			16);
		CameraGetCapability(m_hCamera, &m_sCameraInfo);
		CameraSetAeState(m_hCamera, false);
		CameraPlay(m_hCamera);
		printf("相机%d初始化完成\n", m_hCamera);

		for (size_t i = 0; i < 160; i++)
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
			m_BufIdx = m_BufIdx + 1 < 160 ? m_BufIdx + 1 : 0;
		}
	}
	void saveVideo()
	{
		char name[64];
		sprintf_s(name, "C:/CameraRecord/sucai%d.avi", m_hCamera);
		CameraInitRecord(m_hCamera, 1, name, true, 60, 60);
		int i = m_BufIdx;
		do
		{
			CameraPushFrame(m_hCamera,m_Buffer[i].pFrameBuffer,&m_Buffer[i].pFrameHead);
			i = i + 1 < 160 ? i + 1 : 0;
		} while (i == m_BufIdx);
		CameraStopRecord(m_hCamera);
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	CameraSdkInit(1);
	CameraEnumerateDevice(cameraInfo, &cameraNum);

	int length = 0;
	char* com_num;
	get_com_port(&com_num, length);
	if (open_com_port(com_num))
	{
		printf("打开小盒子\n");
	}
	set_threshold(800);

	Camera* pCamera1 = new Camera();
	Camera* pCamera2 = new Camera();
	pCamera1->init(0);
	pCamera2->init(1);

	bool isVideoOK = false;
	int cueMsg = 0;
	while (true)
	{
		int msg = fetch_msg();
		printf("消息：%d\n", msg);
		if (isVideoOK || msg == 0||cueMsg == msg)
		{
			continue;
		}
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

