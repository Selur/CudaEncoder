/*
 * EncoderHandler.cpp
 *
 *  Created on: 13.08.2010
 *      Author: Selur
 */

#include "EncoderHandler.h"
#include <QString>
#include <QFile>
#include <QtGlobal>
#include "EncoderParameter.h"
#include <fcntl.h>
#include <io.h>
#include "nvidiaheader/NVEncoderAPI.h"
#include "MyTimer.h"
#include <stdio.h>
#include <iostream>
using namespace std;

unsigned char* charbuf;
QFile *outputFile = NULL;
unsigned int cnt;

EncoderHandler::EncoderHandler(QObject *parent) :
    QObject(parent), firstRun(true), configured(false), started(false), bufferTwo(false),
        lastFrame(false), measureFPS(true), frame_timer(NULL), global_timer(NULL), g_FrameCount(0),
        g_fpsCount(0), g_fpsLimit(16), width(0), height(0), readSize(0), showFramestats(0),
        bufOne(NULL), bufTwo(NULL), inputFile(NULL), hNVEncoder(NULL), colorformat(IYUV),
        pData(NULL), hr(S_OK), dwStartTime(0)
{
}

EncoderHandler::~EncoderHandler()
{

}

void EncoderHandler::toCerr(QString value)
{
  value += "\n";
  cerr << qPrintable(value);
}

bool EncoderHandler::SetCodecType(void *hNVEncoder, EncoderParameter *encParameter)
{
  int codec = encParameter->getCodecType();
  HRESULT hr = NVSetCodec(hNVEncoder, codec);
  if (hr != S_OK) {
    toCerr(QObject::tr("ERROR: Set codec type failed!"));
    return false;
  }
  if (codec == 4) {
    toCerr(QObject::tr("INFO: Using H.264 encoder,... "));
  } else {
    toCerr(QObject::tr("ERROR: Unsupported encoder: %1").arg(codec));
    return false;
  }
  return true;
}

HRESULT EncoderHandler::GetParamValue(void *hNVEncoder, DWORD dwParamType, void *pData)
{
  HRESULT hr = S_OK;
  hr = NVGetParamValue(hNVEncoder, dwParamType, pData);
  if (hr != S_OK) {
    toCerr(QObject::tr("ERROR: NVGetParamValue failed for: "));
    cerr << sNVVE_EncodeParams[dwParamType].name;
  }
  return hr;
}

void EncoderHandler::DisplayGPUCaps(void *hNVEncoder, int deviceIndex)
{
  NVVE_GPUAttributes GPUAttributes = { 0 };
  HRESULT hr = S_OK;

  GPUAttributes.iGpuOrdinal = deviceIndex;
  hr = this->GetParamValue(hNVEncoder, NVVE_GET_GPU_ATTRIBUTES, &GPUAttributes);
  if (hr != S_OK) {
    toCerr(QObject::tr("ERROR: NVVE_GET_GPU_ATTRIBUTES error!"));
  }
  toCerr(QObject::tr("INFO:   GPU Device %1      : %2").arg(deviceIndex).arg(GPUAttributes.cName));
  toCerr(
      QObject::tr("INFO:     Compute Capability = SM %1.%2").arg(GPUAttributes.iMajor).arg(
          GPUAttributes.iMinor));
  toCerr(
      QObject::tr("INFO:     Total Memory       = %1 MBytes").arg(
          (float) GPUAttributes.uiTotalGlobalMem / (1024.0f * 1024.0f)));
  toCerr(QObject::tr("INFO:     GPU Clock          = %1 Hz").arg(GPUAttributes.iClockRate));
  toCerr(QObject::tr("INFO:     Multiprocessors    = %1").arg(GPUAttributes.iMultiProcessorCount));
  toCerr(QObject::tr("INFO:     GPU Encoding Mode:"));
  switch (GPUAttributes.MaxGpuOffloadLevel) {
    case NVVE_GPU_OFFLOAD_DEFAULT :
      toCerr(QObject::tr("INFO:       CPU: PEL Processing Only"));
      break;
    case NVVE_GPU_OFFLOAD_ESTIMATORS :
      toCerr(QObject::tr("INFO:       CPU: PEL Processing"));
      toCerr(QObject::tr("INFO:       GPU: Motion Estimation, Intra Prediction"));
      break;
    case NVVE_GPU_OFFLOAD_ALL :
      toCerr(QObject::tr("INFO:       CPU: Entropy Encoding"));
      toCerr(QObject::tr("INFO:       GPU: Full Offload of Encoding"));
      break;
  }
}

HRESULT EncoderHandler::SetParamValue(void *hNVEncoder, DWORD dwParamType, void *pData)
{
  HRESULT hr = S_OK;
  hr = NVSetParamValue(hNVEncoder, dwParamType, pData);
  QString message;
  if (hr != S_OK) {
    message = QObject::tr("ERROR:  NVSetParamValue %1 failed,  hr = %2").arg(
        sNVVE_EncodeParams[dwParamType].name).arg(hr);
    for (int i = 0; i < sNVVE_EncodeParams[dwParamType].params; i++) {
      message += "   " + QObject::tr("%1").arg(*((DWORD *) pData + i));
      message += ", ";
    }
  } else {
    message = QObject::tr("%1 ").arg(sNVVE_EncodeParams[dwParamType].name);
    for (int i = 0; i < sNVVE_EncodeParams[dwParamType].params; i++) {
      message += "   " + QObject::tr("%1").arg(*((DWORD *) pData + i));
    }
  }

  switch (dwParamType) {
    case NVVE_PRESETS :
      message += " " + QObject::tr("[%1 Profile]").arg(sVideoEncodePresets[*((DWORD *) pData)]);
      break;
    case NVVE_GPU_OFFLOAD_LEVEL :
      switch (sNVVE_EncodeParams[dwParamType].params) {
        case -1 :
          message += " " + QObject::tr("[%s]").arg(sGPUOffloadLevel[0]);
          break;
        case 8 :
          message += " " + QObject::tr("[%s]").arg(sGPUOffloadLevel[1]);
          break;
        case 16 :
          message += " " + QObject::tr("[%s]").arg(sGPUOffloadLevel[2]);
          break;
      }
      break;
  }
  if (!message.isEmpty()) {
    toCerr("PARAM: "+message);
  }
  return hr;
}

bool EncoderHandler::SetEncodeParams(void *hNVEncoder, EncoderParameter *encParameter)
{
  int intTwo[2] = { encParameter->getInputSizeWidth(), encParameter->getInputSizeHeight() };
  HRESULT hr = this->SetParamValue(hNVEncoder, NVVE_OUT_SIZE, &(intTwo));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set output resolution!"));
    return false;
  }
  int intThree[3] =
    { encParameter->getAspectRatioWidth(), encParameter->getAspectRatioHeight(), encParameter
          ->getAspectRatioType() };
  hr = this->SetParamValue(hNVEncoder, NVVE_ASPECT_RATIO, &(intThree));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set aspect ratio!"));
    return false;
  }
  int intOne = encParameter->getFieldMode();
  hr = this->SetParamValue(hNVEncoder, NVVE_FIELD_ENC_MODE, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set field encoding mode!"));
    return false;
  }
  intOne = encParameter->getP_Interval();
  hr = this->SetParamValue(hNVEncoder, NVVE_P_INTERVAL, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set p-frame interval!"));
    return false;
  }
  intOne = encParameter->getIDR_Period();
  hr = this->SetParamValue(hNVEncoder, NVVE_IDR_PERIOD, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set idr-frame interval!"));
    return false;
  }
  intOne = encParameter->getDynamicGOP();
  hr = this->SetParamValue(hNVEncoder, NVVE_DYNAMIC_GOP, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set dynamic/static gop decision!"));
    return false;
  }
  intOne = encParameter->getRCType();
  hr = this->SetParamValue(hNVEncoder, NVVE_RC_TYPE, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set rate control type!"));
    return false;
  }
  intOne = encParameter->getAvgBitrate();
  hr = this->SetParamValue(hNVEncoder, NVVE_AVG_BITRATE, &(intOne));
  if (hr != S_OK) {
    return false;
  }
  intOne = encParameter->getPeakBitrate();
  hr = this->SetParamValue(hNVEncoder, NVVE_PEAK_BITRATE, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set peak bitrate!"));
    return false;
  }
  intOne = encParameter->getQP_Level_Intra();
  hr = this->SetParamValue(hNVEncoder, NVVE_QP_LEVEL_INTRA, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set qp intra level!"));
    return false;
  }
  intOne = encParameter->getQP_Level_InterP();
  hr = this->SetParamValue(hNVEncoder, NVVE_QP_LEVEL_INTER_P, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set qp inter(p) level!"));
    return false;
  }
  intOne = encParameter->getQP_Level_InterB();
  hr = this->SetParamValue(hNVEncoder, NVVE_QP_LEVEL_INTER_B, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set qp inter(b) level!"));
    return false;
  }
  intTwo[0] = encParameter->getFrameRateNumerator();
  intTwo[1] = encParameter->getFrameRateDenominator();
  hr = this->SetParamValue(hNVEncoder, NVVE_FRAME_RATE, &(intTwo));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set frame rate!"));
    return false;
  }
  intOne = encParameter->getDeblockMode();
  hr = this->SetParamValue(hNVEncoder, NVVE_DEBLOCK_MODE, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set deblock mode!"));
    return false;
  }
  intOne = encParameter->getProfileLevel();
  hr = this->SetParamValue(hNVEncoder, NVVE_PROFILE_LEVEL, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set profile and level!"));
    return false;
  }
  intOne = encParameter->getDiMode();
  hr = this->SetParamValue(hNVEncoder, NVVE_SET_DEINTERLACE, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set deinterlace value!"));
    return false;
  }
  intOne = encParameter->getPresetsTarget();
  if (intOne != -1) {
    hr = this->SetParamValue(hNVEncoder, NVVE_PRESETS, &(intOne));
    if (hr != S_OK) {
      toCerr(QObject::tr("Couldn't set NVE preset!"));
      return false;
    }
  }
  intOne = encParameter->getDisableCabac();
  hr = this->SetParamValue(hNVEncoder, NVVE_DISABLE_CABAC, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set CABAC/CAVLC!"));
    return false;
  }
  intOne = encParameter->getConfigNaluFramingType();
  hr = this->SetParamValue(hNVEncoder, NVVE_CONFIGURE_NALU_FRAMING_TYPE, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set nalu framing type!"));
    return false;
  }
  intOne = encParameter->getDisableSPSPPS();
  hr = this->SetParamValue(hNVEncoder, NVVE_DISABLE_SPS_PPS, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't dis-/enable sps/pps !"));
    return false;
  }
  intOne = encParameter->getSliceCount();
  hr = this->SetParamValue(hNVEncoder, NVVE_SLICE_COUNT, &(intOne));
  if (hr != S_OK) {
    toCerr(QObject::tr("Couldn't set slice count!"));
    return false;
  }
  return true;
}

bool EncoderHandler::StartPerfLog(DWORD *pdwElapsedTime, LARGE_INTEGER *pliUserTime0,
                                  LARGE_INTEGER *pliKernelTime0)
{
  *pdwElapsedTime = timeGetTime();
  FILETIME ftCreationTime, ftExitTime, ftKernelTime, ftUserTime;
  if (GetProcessTimes(GetCurrentProcess(), &ftCreationTime, &ftExitTime, &ftKernelTime,
                      &ftUserTime)) {
    pliUserTime0->LowPart = ftUserTime.dwLowDateTime;
    pliUserTime0->HighPart = ftUserTime.dwHighDateTime;
    pliKernelTime0->LowPart = ftKernelTime.dwLowDateTime;
    pliKernelTime0->HighPart = ftKernelTime.dwHighDateTime;
  } else {
    toCerr(QObject::tr("ERROR: Error in measuring CPU utilization"));
  }
  return true;
}

// signals the start of bitstream that is to be encoded
static unsigned char* HandleAcquireBitStream(int *pBufferSize, void *)
{
  *pBufferSize = 10240 * 1024;
  return charbuf;
}

//signals that the encoded bitstream is ready to be written to file
static void HandleReleaseBitStream(int nBytesInBuffer, unsigned char *buffer, void *)
{
  if (outputFile != NULL) {
    //cerr << qPrintable(QObject::tr("writing encoded bitstream to file,..")) << endl;
    outputFile->write((char *) buffer, nBytesInBuffer);
  }
}

//signals that the encoding operation on the frame has begun
static void HandleOnBeginFrame(const NVVE_BeginFrameInfo *, void *)
{
  //cerr << qPrintable(QObject::tr("encoding operation on the frame has begun")) << endl;
  return;
}

//signals that the encoding operation on the frame has ended
static void HandleOnEndFrame(const NVVE_EndFrameInfo *, void *)
{
  //cerr << qPrintable(QObject::tr("Encoding frame %1").arg(cnt)) << endl;
  cnt++;
}

void EncoderHandler::computeFPS()
{
  this->frame_timer->stop();
  g_FrameCount++;
  g_fpsCount++;
  if (g_fpsCount == showFramestats) {
    float ifps = 1.f / (this->frame_timer->getAverageTime() / 1000.f);
    toCerr(
        QObject::tr("[Frame: %1, %2 fps, frame time: %3 (ms)]").arg(g_FrameCount).arg(ifps).arg(
            1000.f / ifps));
    this->frame_timer->reset();
    g_fpsCount = 0;
  }
  this->frame_timer->start();
}

bool EncoderHandler::StopPerfLog(DWORD dwStartTime, void *)
{
  DWORD dwElapsedTime = 0;
  dwElapsedTime = timeGetTime() - dwStartTime;
  toCerr(QObject::tr("INFO: Number of Coded Frames        : %1").arg(cnt));
  toCerr(QObject::tr("INFO: Elapsed time                  : %1 ms").arg(dwElapsedTime));
  toCerr(QObject::tr("INFO: End to End FPS                : %1").arg(
          (float) ((double) cnt / ((double) dwElapsedTime / 1000.0f))));

  //CPU utilization
  FILETIME ftCreationTime, ftExitTime, ftKernelTime, ftUserTime;
  if (GetProcessTimes(GetCurrentProcess(), &ftCreationTime, &ftExitTime, &ftKernelTime,
                      &ftUserTime)) {
    LARGE_INTEGER liUserTime, liKernelTime;
    SYSTEM_INFO si;
    LONG nCores;
    memset(&si, 0, sizeof(si));
    GetSystemInfo(&si);
    nCores = max(int(si.dwNumberOfProcessors), 1);
    liUserTime.LowPart = ftUserTime.dwLowDateTime;
    liUserTime.HighPart = ftUserTime.dwHighDateTime;
    liKernelTime.LowPart = ftKernelTime.dwLowDateTime;
    liKernelTime.HighPart = ftKernelTime.dwHighDateTime;
    toCerr(
        QObject::tr("INFO: CPU utilization               : %1").arg(
            ((double) ((liUserTime.QuadPart - liUserTime0.QuadPart)
                + (liKernelTime.QuadPart - liKernelTime0.QuadPart)) / (100 * nCores))
                / (double) dwElapsedTime));
    toCerr(
        tr("(user: %1%, kernel: %2%) / %3 cores").arg(
            ((double) (liUserTime.QuadPart - liUserTime0.QuadPart) / 1000) / (double) dwElapsedTime)
            .arg(
            ((double) (liKernelTime.QuadPart - liKernelTime0.QuadPart) / 100)
                / (double) dwElapsedTime).arg(nCores));
  } else {
    toCerr(QObject::tr("ERROR: Error in measuring CPU utilization"));
  }

  return true;
}

void EncoderHandler::configureEncoder(EncoderParameter *encParameter)
{
  QString fileName = encParameter->getInputFileName();
  cnt = 0;
  if (fileName == "-") {
    int fd = _fileno(stdin);
    if (-1 == _setmode(fd, _O_BINARY)) {
      toCerr(QObject::tr("Couldn't reset file mode to binary."));
      this->exit(-12);
      return;
    }
    inputFile = new QFile(this);
    if (!inputFile->open(fd, QIODevice::ReadOnly)) {
      toCerr(QObject::tr("ERROR: Couldn't get read only access to stdIn,..."));
      this->exit(-1);
      return;
    }
    toCerr(QObject::tr("INFO: Reading input from stdIn,..."));
  } else {
    inputFile = new QFile(fileName);
    if (!inputFile->open(QIODevice::ReadOnly)) {
      toCerr(QObject::tr("ERROR: Couldn't get read only access to: ").arg(fileName));
      this->exit(-2);
      return;
    }
    toCerr(QObject::tr("INFO: Reading input from: %1").arg(fileName));
  }

  fileName = encParameter->getOutputFileName();
  outputFile = new QFile(fileName, this);
  if (!outputFile->open(QIODevice::WriteOnly)) {
    toCerr(QObject::tr("ERROR: Couldn't get write only access to: %1").arg(fileName));
    this->exit(-3);
    return;
  }

  toCerr(QObject::tr("INFO: Create the timer for frame time measurement,.."));
  // Create the timer for frame time measurement
  this->frame_timer = new MyTimer();
  this->frame_timer->reset();
  this->global_timer = new MyTimer();
  this->global_timer->reset();
  toCerr(QObject::tr("INFO: Creating encoder api interface,.."));

  hr = NVCreateEncoder(&hNVEncoder); //create the encoder api interface
  if (hr != S_OK) {
    toCerr(QObject::tr("NVCreateEncoder failed to create the NV Encoder API handle"));
    this->exit(-4);
    return;
  }
  toCerr(QObject::tr("INFO: Created a NVEncoder instance,.."));
  if (!this->SetCodecType(hNVEncoder, encParameter)) { // Set encoder codec format
    toCerr(QObject::tr("Coudln't set encoder codec format."));
    this->exit(-5);
    return;
  }
  int deviceCount = encParameter->getGPUCount();
  hr = this->GetParamValue(hNVEncoder, NVVE_GET_GPU_COUNT, &deviceCount);
  if (hr == E_NOTIMPL) {
    toCerr(QObject::tr("The parameter is not supported,..."));
    this->exit(-6);
  } else if (hr == E_UNEXPECTED) {
    toCerr(QObject::tr("The encoding format is not initialized."));
    this->exit(-6);
  } else if (hr == E_POINTER) {
    toCerr(QObject::tr("&deviceCount is NULL pointer"));
    this->exit(-6);
  }
  toCerr(QObject::tr("INFO: Detected %1 GPU(s) capable of GPU Encoding.").arg(deviceCount));

  for (int i = 0; i < deviceCount; i++) {
    this->DisplayGPUCaps(hNVEncoder, i);
  }

  if (encParameter->getForceDevice()) {
    int force = encParameter->getForcedGPU();
    toCerr(QObject::tr("INFO: Choosing GPU with index %1 for encoding.").arg(force));
    hr = this->SetParamValue(hNVEncoder, NVVE_FORCE_GPU_SELECTION, &force);
    if (hr != S_OK) {
      toCerr(QObject::tr("ERROR: Couldn't select GPU with index %1 for encoding.").arg(force));
      this->exit(-7);
      return;
    }
  } else {
    toCerr(QObject::tr("INFO: Using device with index 0,..."));
  }

  int offloadLevel = encParameter->getGpuOffload();
  hr = this->SetParamValue(hNVEncoder, NVVE_GPU_OFFLOAD_LEVEL, &(offloadLevel));
  if (hr != S_OK) {
    toCerr(QObject::tr("ERROR: Couldn't set the GPU offload level!"));
    this->exit(-8);
    return;
  }

  bool bRetVal = this->SetEncodeParams(hNVEncoder, encParameter);
  if (!bRetVal) {
    toCerr(QObject::tr("ERROR: Set encoder parameters failed!"));
    this->exit(-9);
    return;
  }

  this->StartPerfLog(&dwStartTime, &liUserTime0, &liKernelTime0); //calculate fps and cpu utilization

  NVVE_CallbackParams nvcb;
  memset(&nvcb, 0, sizeof(nvcb));
  nvcb.pfnacquirebitstream = (PFNACQUIREBITSTREAM) HandleAcquireBitStream;
  nvcb.pfnonbeginframe = (PFNONBEGINFRAME) HandleOnBeginFrame;
  nvcb.pfnonendframe = (PFNONENDFRAME) HandleOnEndFrame;
  nvcb.pfnreleasebitstream = (PFNRELEASEBITSTREAM) HandleReleaseBitStream;
  toCerr(QObject::tr("INFO: Register the callback structure,..."));
  NVRegisterCB(hNVEncoder, nvcb, pData); //register the callback structure
  toCerr(QObject::tr("INFO: Create the hw resources for encoding.."));
  hr = NVCreateHWEncoder(hNVEncoder); //create the hw resources for encoding on nvidia hw
  if (hr != S_OK) {
    toCerr(QObject::tr("ERROR: Failed to create the Nvidia HW encoder to encode this stream!"));
    this->exit(-10);
    return;
  }
  toCerr(QObject::tr("INFO: Starting encoding,..."));
  charbuf = (unsigned char *) malloc(10240 * 1024 * sizeof(char));
  int memSize = int(
      encParameter->getInputSizeWidth() * encParameter->getInputSizeHeight() * (1.5f) + 0.5);
  bufOne = (char *) malloc(memSize * sizeof(char) + 1);
  bufTwo = (char *) malloc(memSize * sizeof(char) + 1);
  unsigned char buf2[10];
  int size;
  hr = NVGetSPSPPS(hNVEncoder, buf2, 10, &size);
  if (hr != S_OK) {
    toCerr(QObject::tr("ERROR: Failed to get SPSPPS buffer."));
    this->exit(-11);
    return;
  }
  readSize = memSize;
  width = encParameter->getInputSizeWidth();
  height = encParameter->getInputSizeHeight();
  colorformat = encParameter->getColorFormat();

  switch (colorformat) {
    case 0 :
      toCerr(QObject::tr("INFO: Colorspace: UYVY"));
      break;
    case 1 :
      toCerr(QObject::tr("INFO: Colorspace: YUY2"));
      break;
    case 2 :
      toCerr(QObject::tr("INFO: Colorspace: YV12"));
      break;
    case 3 :
      toCerr(QObject::tr("INFO: Colorspace: NV12"));
      break;
    case 4 :
      toCerr(QObject::tr("INFO: Colorspace: IYUV"));
      break;
    default :
      break;
  }
  measureFPS = encParameter->getMeasure_fps();
  toCerr(QObject::tr("INFO: measuring FPS: %1").arg((measureFPS) ? "true" : "false"));
  showFramestats = encParameter->getShowFramestats();
  toCerr(QObject::tr("INFO: showFramestats: %1").arg(showFramestats));
  configured = true;
}

int EncoderHandler::exit(int retValue)
{
  toCerr(QObject::tr("INFO: Encoder handler exited with %1").arg(retValue));
  toCerr(QObject::tr("INFO: Closing cuda,..."));
  if (inputFile != NULL) {
    toCerr(QObject::tr("INFO: Closing input file reader,..."));
    inputFile->close();
  }
  if (hNVEncoder) {
    toCerr(QObject::tr("INFO: Destroying cuda encoder,..."));
    HRESULT hr = NVDestroyEncoder(hNVEncoder);
    if (hr == E_POINTER) {
      toCerr(QObject::tr("ERROR: Encoder handle is invalid,..."));
    }
    hNVEncoder = NULL;
  }
  if (outputFile != NULL) {
    toCerr(QObject::tr("INFO: Closing output file writer,..."));
    outputFile->close();
  }
  if (bufOne != NULL) {
    toCerr(QObject::tr("INFO: Freeing file buffer one,..."));
    free(bufOne);
    bufOne = NULL;
  }
  if (bufTwo != NULL) {
    toCerr(QObject::tr("INFO: Freeing file buffer two,..."));
    free(bufTwo);
    bufTwo = NULL;
  }
  if (charbuf != NULL) {
    toCerr(QObject::tr("INFO: Freeing char buffer,..."));
    free(charbuf);
    charbuf = NULL;
  }
  toCerr(QObject::tr("Cuda returned with return value: %1").arg(retValue));
  return retValue;
}

char * EncoderHandler::readFrame()
{
  if (firstRun) {
    firstRun = false;
    readFrame();
  }
  if (lastFrame) {
    return NULL;
  }
  char * buffer;
  if (bufferTwo) {
    buffer = bufOne;
  } else {
    buffer = bufTwo;
  }

  int read, toRead = readSize;
  while (toRead > 0) {
    read = inputFile->read(buffer + (readSize - toRead), toRead);
    if (read == 0) {
      lastFrame = true;
      break;
    }
    toRead -= read;
  }
  buffer[(readSize - toRead)] = 0;

  if (bufferTwo) {
    bufferTwo = false;
    return bufTwo;
  } else {
    bufferTwo = true;
    return bufOne;
  }
}

int EncoderHandler::startEncoding()
{
  if (!configured) {
    toCerr(QObject::tr("ERROR: Configuration failed,..."));
    return this->exit(-13);
  }
  if (started) {
    toCerr(QObject::tr("ERROR: Already encoding,..."));
    return this->exit(-14);
  }

  HRESULT hr = NVGetHWEncodeCaps();
  if (hr == E_FAIL) {
    toCerr(QObject::tr("ERROR: No CUDA capability present,..."));
    return this->exit(-15);
  }
  started = true;
  this->global_timer->start();
  this->global_timer->reset();
  this->frame_timer->start();
  this->frame_timer->reset();
  char *buffer = this->readFrame();
  if (buffer == NULL) {
     toCerr(QObject::tr("Input buffer is empty,..."));
     this->global_timer->stop();
     this->frame_timer->stop();
     return this->exit(-1);
  }
  NVVE_EncodeFrameParams efparams;
  efparams.Height = height;
  efparams.Width = width;
  efparams.Pitch = width;
  efparams.PictureStruc = FRAME_PICTURE;
  efparams.SurfFmt = colorformat;
  efparams.progressiveFrame = 1;
  efparams.repeatFirstField = 0;
  efparams.topfieldfirst = 1;
  toCerr(QObject::tr("Starting the encoding,..."));
  while (buffer != NULL) {
    efparams.bLast = lastFrame;
    efparams.picBuf = (unsigned char*) buffer; //get the yuv buffer pointer from file
    hr = NVEncodeFrame(hNVEncoder, &efparams, -1, pData); //actually send the buffer to encode
    if (hr != S_OK) {
      if (hr == E_FAIL) {
        toCerr(QObject::tr("Failed to encode frame %1").arg(cnt));
      } else if (hr == E_POINTER) {
        toCerr(QObject::tr("Encoder handle is invalid,..."));
      }
    }
    if (measureFPS) {
      computeFPS();
    }
    buffer = readFrame();
  }
  toCerr(QObject::tr("Finished encoding,..."));
  this->global_timer->stop();

  if (measureFPS) {
    StopPerfLog(dwStartTime, hNVEncoder);
  }

  toCerr(QObject::tr("Closing cuda,..."));
  return this->exit(0);
}

void EncoderHandler::removeQuotes(QString &input)
{
  input = input.trimmed();
  if (!input.startsWith("\"") || !input.endsWith("\"")) {
    return;
  }
  input = input.remove(0, 1);
  input = input.remove(input.size() - 1, 1);
}

void EncoderHandler::printHelp()
{
  cout << "Authors: netcasthd&Selur can both be contacted through the doom9 forum";
  cout << qPrintable(
          QObject::tr(
              "Authors: netcasthd&Selur can both be contacted through the doom9 forum\n"
              "Usage: cuda --input \"path to input file\" --resolution WIDTHxHEIGHT --output \"path to output file\"\n\
example: 'cuda --input test.yuv --resolution 640x352 --output test.264'\n\
\n\
Required Settings:\n\
-----------------------\n\
--resolution    <value>x<value>   specify the resolution of the input file (needs to be at least mod2)\n\
--output        <value>           specify the input file (the encoder doesn't care about the extension)\n\
\n\
Global Settings:\n\
-----------------------\n\
--input         <value>           specify the raw input file or - for pipe input (-)\n\
--sar           <value>x<value>   set the pixel aspect ratio, e.g. 1x1, 16x11 (1x1)\n\
--format        <value>           set input color format:  UYVY, YUY2, YV12, NV12, IYUV (IYUV)\n\
--control_mode  <value>           select the rate control method:\n\
                                  allowed for h.264:  cbr, vbr, cq, vbr_rest (vbr_rest)\n\
                                  allowed for vc-1:   cbr (cbr)\n\
--bitrate       <value>           target bitrate kBit/s (1500) - only for cbr, vbr, vbr_rest\n\
--bitrate_peak  <value>           maximal bitrate peak kBit/s (62000) - only for cbr, vbr, vbr_rest\n\
--fps <value>                     set the ouput frame rate (25/1)\n\
--fps <numerator/denominator>\n\
--profile       <value>           select a profile for the output stream:\n\
                                  allowed for h.264:  baseline, main, high (high)\n\
                                  allowed for vc-1:   simple, main (main)\n\
--level         <value>           select a profile for the output stream:\n\
                                  allowed for h.264:  10,11,12,13,20,21,22,30,31,32,40,41,42,50,51 (auto)\n\
                                  allowed for vc-1:   low, medium, high, auto (auto)\n\
--offload       <value>           specify gpu work offload, values: partial, full (partial)\n\
--forceGPU      <value>           set if you want to force a specific gpu to be used: 0/1 (NA)\n\
--measure       <value>           during encoding measure: FPS, NONE (FPS)\n\
--showFrameStats <value>          set how often frame stats should be posted during encoding: 0- (0 = disable)\n\
                                  (only when measure FPS is used)\n\
--frame_typ     <value>           set the outptu frame typ: frame, top, bottom (frame)\n\
--pframe_dist   <value>           set the minimum distance between two p-frames (1)\n\
--gop_max       <value>           set the maximum distance between two key frames (250)\n\
--dynamicGOP    <value>           set whether or not gop structure should be choosen dynamically: true/false (true)\n\
--pquant_min    <value>           set a min quantizer for p-frames (0) - only with vbr_rest\n\
--bquant_min    <value>           set a min quantizer for b-frames (0) - only with vbr_rest\n\
--iquant_min    <value>           set a min quantizer for i-frames (0) - only with vbr_rest\n\
--deblock       <value>           dis-/enable deblocking: true/false (true)\n\
--deinterlace   <value>           dis-/enable deinterlacing: true/false (false)\n\
--preset        <value>           select a preset: psps, ipod, avchd, bluray, hdv1440\n\
--cavlc         <value>           use CAVLC instead of CABAC entropy coding\n\
--nal_typ       <value>           select nal-unit type: auto, 1-4 (auto)\n\
--sps_pps       <value>           dis-/enable sps_pps flaf (true)\n\
--slices        <value>           set slice count: auto, 1-4\n"));
}
