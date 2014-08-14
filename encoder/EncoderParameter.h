/*
 * EncoderParameter.h
 *
 *  Created on: 09.08.2010
 *      Author: Selur
 */

#ifndef ENCODERPARAMETER_H_
#define ENCODERPARAMETER_H_

#include "nvidiaheader/mywindows.h"
#include "nvidiaheader/NVEncodeDataTypes.h"
#include <QString>

class EncoderParameter
{
  public:
    EncoderParameter();
    ~EncoderParameter();
    bool setEncoderParameter(int argc, char *argv[]);

    //GETTER
    int getAspectRatioHeight() const;
    NVVE_ASPECT_RATIO_TYPE getAspectRatioType() const;
    int getAspectRatioWidth() const;
    int getAvgBitrate() const;
    int getCPU_count() const;
    int getCodecType() const;
    int getConfigNaluFramingType() const;
    int getDeblockMode() const;
    NVVE_DI_MODE getDiMode() const;
    int getDisableCabac() const;
    int getDisableSPSPPS() const;
    int getDynamicGOP() const;
    NVVE_FIELD_MODE getFieldMode() const;
    int getForcedGPU() const;
    int getFrameRateDenominator() const;
    int getFrameRateNumerator() const;
    int getGPU_devID() const;
    NVVE_GPUOffloadLevel getGpuOffload() const;
    int getIDR_Period() const;
    QString getInputFileName() const;
    int getInputSizeHeight() const;
    int getInputSizeWidth() const;
    bool getMeasure_fps() const;
    bool getMeasure_psnr() const;
    QString getOutputFileName() const;
    int getOutputSize() const;
    int getP_Interval() const;
    int getPeakBitrate() const;
    int getPresetsTarget() const;
    int getProfileLevel() const;
    int getQP_Level_InterB() const;
    int getQP_Level_InterP() const;
    int getQP_Level_Intra() const;
    NVVE_RateCtrlType getRCType() const;
    int getSliceCount() const;
    int getGPUCount() const;
    int getForceDevice() const;
    NVVE_SurfaceFormat getColorFormat() const;
    int getShowFramestats() const;

    //SETTER
    void setColorFormat(NVVE_SurfaceFormat colorFormat);
    void setAspectRatioHeight(int iAspectRatioHeight);
    void setAspectRatioType(NVVE_ASPECT_RATIO_TYPE aspectRatioType);
    void setAspectRatioWidth(int iAspectRatioWidth);
    void setAvgBitrate(int iAvgBitrate);
    void setCPU_count(int iCPU_count);
    void setClearStat(int iClearStat);
    void setCodecType(int iCodecType);
    void setConfigNaluFramingType(int iConfigNaluFramingType);
    void setDeblockMode(int iDeblockMode);
    void setDiMode(NVVE_DI_MODE diMode);
    void setDisableCabac(int iDisableCabac);
    void setDisableSPSPPS(int iDisableSPSPPS);
    void setDynamicGOP(int iDynamicGOP);
    void setFieldMode(NVVE_FIELD_MODE fieldMode);
    void setForceIDR(int iForceIDR);
    void setForceIntra(int iForceIntra);
    void setForcedGPU(int iForcedGPU);
    void setFrameRateDenominator(int iFrameRateDenominator);
    void setFrameRateNumerator(int iFrameRateNumerator);
    void setGPU_devID(int iGPU_devID);
    void setGpuOffload(NVVE_GPUOffloadLevel gpuOffload);
    void setIDR_Period(int iIDR_Period);
    void setInputFileName(QString inputFileName);
    void setInputSizeHeight(int iInputSizeHeight);
    void setInputSizeWidth(int iInputSizeWidth);
    void setMeassure_fps(bool measure_fps);
    void setMeassure_psnr(bool measure_psnr);
    void setOutputFileName(QString outputFileName);
    void setP_Interval(int iP_Interval);
    void setPeakBitrate(int iPeakBitrate);
    void setPresetsTarget(NVVE_PRESETS_TARGET presetsTarget);
    void setProfileLevel(int iProfileLevel);
    void setQP_Level_InterB(int iQP_Level_InterB);
    void setQP_Level_InterP(int iQP_Level_InterP);
    void setQP_Level_Intra(int iQP_Level_Intra);
    void setRCType(NVVE_RateCtrlType RCType);
    void setSliceCount(int iSliceCount);

    void setResolution(QString resolution);
    void setFramerate(const QString framerate);
    void setGPUCount(int iGPUCount);
    void setForceDevice(int forceDevice);
    void setShowFramestats(int showFramestatsdist);

  private:
    QString inputFileName;
    QString outputFileName;
    bool measure_psnr;
    bool measure_fps;
    int iCPU_count;
    int iGPU_devID;
    NVVE_GPUOffloadLevel gpuOffload;
    int iGPUCount;
    int iForcedGPU;
    int forceDevice;
    int iCodecType;
    int iAspectRatioWidth;
    int iAspectRatioHeight;
    NVVE_ASPECT_RATIO_TYPE aspectRatioType;
    NVVE_FIELD_MODE fieldMode;
    int iP_Interval;
    int iIDR_Period;
    int iDynamicGOP;
    NVVE_RateCtrlType RCType;
    int iAvgBitrate;
    int iPeakBitrate;
    int iQP_Level_Intra;
    int iQP_Level_InterP;
    int iQP_Level_InterB;
    int iFrameRateNumerator;
    int iFrameRateDenominator;
    int iDeblockMode;
    int iProfileLevel;
    NVVE_DI_MODE diMode;
    int presetsTarget;
    int iInputSizeWidth;
    int iInputSizeHeight;
    int iDisableCabac;
    int iConfigNaluFramingType;
    int iDisableSPSPPS;
    int iSliceCount;
    NVVE_SurfaceFormat colorFormat;
    int showFramestats;
    void setDefaults();
    bool checkSettings();
    QString getLevelForBitrate(int bitrate);
    QString getCurrentLevel();
    QString getCurrentProfile();
    bool levelAsmallerB(QString a, QString b);
    QString getMinLevelForFrameSize();
    void toCerr(QString value);
};

static char *sGPUOffloadLevel[] =
  { "GPU Offload (PEL processing)", "GPU Offload (Motion Esimation)", "GPU Offload (Full Encoder)",
    NULL };

static char *sVideoEncodePresets[] =
  { "PSP         ( 320x 240)", "iPod/iPhone ( 320x 240)", "AVCHD       (1920x1080)",
    "BluRay      (1920x1080)", "HDV_1440    (1440x1080)", "ZuneHD", "FlipCam", NULL };

typedef struct
{
    char *name;
    int params;
} _sNVVEEncodeParams;

static _sNVVEEncodeParams sNVVE_EncodeParams[] =
  {
    { "UNDEFINED", 1 },
      { "NVVE_OUT_SIZE", 2 },
      { "NVVE_ASPECT_RATIO", 3 },
      { "NVVE_FIELD_ENC_MODE", 1 },
      { "NVVE_P_INTERVAL", 1 },
      { "NVVE_IDR_PERIOD", 1 },
      { "NVVE_DYNAMIC_GOP", 1 },
      { "NVVE_RC_TYPE", 1 },
      { "NVVE_AVG_BITRATE", 1 },
      { "NVVE_PEAK_BITRATE", 1 },
      { "NVVE_QP_LEVEL_INTRA", 1 },
      { "NVVE_QP_LEVEL_INTER_P", 1 },
      { "NVVE_QP_LEVEL_INTER_B", 1 },
      { "NVVE_FRAME_RATE", 2 },
      { "NVVE_DEBLOCK_MODE", 1 },
      { "NVVE_PROFILE_LEVEL", 1 },
      { "NVVE_FORCE_INTRA (dshow)", 1 },            //DShow only
      { "NVVE_FORCE_IDR   (dshow)", 1 },            //DShow only
      { "NVVE_CLEAR_STAT  (dshow)", 1 },            //DShow only
      { "NVVE_SET_DEINTERLACE", 1 },
      { "NVVE_PRESETS", 1 },
      { "NVVE_IN_SIZE", 2 },
      { "NVVE_STAT_NUM_CODED_FRAMES (dshow)", 1 },       //DShow only
      { "NVVE_STAT_NUM_RECEIVED_FRAMES (dshow)", 1 },    //DShow only
      { "NVVE_STAT_BITRATE (dshow)", 1 },                //DShow only
      { "NVVE_STAT_NUM_BITS_GENERATED (dshow)", 1 },     //DShow only
      { "NVVE_GET_PTS_DIFF_TIME (dshow)", 1 },           //DShow only
      { "NVVE_GET_PTS_BASE_TIME (dshow)", 1 },           //DShow only
      { "NVVE_GET_PTS_CODED_TIME (dshow)", 1 },          //DShow only
      { "NVVE_GET_PTS_RECEIVED_TIME (dshow)", 1 },       //DShow only
      { "NVVE_STAT_ELAPSED_TIME (dshow)", 1 },           //DShow only
      { "NVVE_STAT_QBUF_FULLNESS (dshow)", 1 },          //DShow only
      { "NVVE_STAT_PERF_FPS (dshow)", 1 },               //DShow only
      { "NVVE_STAT_PERF_AVG_TIME (dshow)", 1 },          //DShow only
      { "NVVE_DISABLE_CABAC", 1 },
      { "NVVE_CONFIGURE_NALU_FRAMING_TYPE", 1 },
      { "NVVE_DISABLE_SPS_PPS", 1 },
      { "NVVE_SLICE_COUNT", 1 },
      { "NVVE_GPU_OFFLOAD_LEVEL", 1 },
      { "NVVE_GPU_OFFLOAD_LEVEL_MAX", 1 },
      { "NVVE_MULTI_GPU", 1 },
      { "NVVE_GET_GPU_COUNT", 1 },
      { "NVVE_GET_GPU_ATTRIBUTES", 1 },
      { "NVVE_FORCE_GPU_SELECTION", 1 },
      { NULL, 0 } };

#endif /* ENCODERPARAMETER_H_ */
