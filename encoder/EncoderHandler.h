/*
 * EncoderHandler.h
 *
 *  Created on: 13.08.2010
 *      Author: Selur
 */

#ifndef ENCODERHANDLER_H_
#define ENCODERHANDLER_H_
#include <QObject>
#include <QString>
#include <QtGlobal>
#include "EncoderParameter.h"
class QFile;
class MyTimer;

class EncoderHandler : public QObject
{
  Q_OBJECT
  public:
    EncoderHandler(QObject *parent = 0);
    ~EncoderHandler();
    void static removeQuotes(QString &input);
    void static printHelp();
    void configureEncoder(EncoderParameter *encParameter);
    int startEncoding();

  private:
    bool firstRun, configured, started, bufferTwo, lastFrame, measureFPS;
    MyTimer *frame_timer, *global_timer;
    unsigned int g_FrameCount, g_fpsCount, g_fpsLimit;
    int width, height, readSize, showFramestats;
    char *bufOne, *bufTwo;
    QFile *inputFile;
    void *hNVEncoder;
    NVVE_SurfaceFormat colorformat;
    void *pData;
    HRESULT hr;
    DWORD dwStartTime;
    LARGE_INTEGER liUserTime0, liKernelTime0;
    int exit(int retValue);
    bool SetCodecType(void *hNVEncoder, EncoderParameter *encParameter);
    HRESULT GetParamValue(void *hNVEncoder, DWORD dwParamType, void *pData);
    void DisplayGPUCaps(void *hNVEncoder, int deviceOrdinal);
    HRESULT SetParamValue(void *hNVEncoder, DWORD dwParamType, void *pData);
    bool SetEncodeParams(void *hNVEncoder, EncoderParameter *encParameter);
    bool StartPerfLog(DWORD *pdwElapsedTime, LARGE_INTEGER *pliUserTime0,
                      LARGE_INTEGER *pliKernelTime0);
    void computeFPS();
    bool StopPerfLog(DWORD dwStartTime, void *hNVEncoder);
    char * readFrame();
    void toCerr(QString value);

};

#endif /* ENCODERHANDLER_H_ */
