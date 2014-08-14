/*
 * EncoderParameter.cpp
 *
 *  Created on: 09.08.2010
 *      Author: Selur
 */

#include "EncoderParameter.h"
#include <QStringList>
#include "EncoderHandler.h"
#include <QFile>
#include <iostream>
using namespace std;
//TODO: add checks for Presets

EncoderParameter::EncoderParameter()
{
  this->setDefaults();
}

EncoderParameter::~EncoderParameter()
{
}

void EncoderParameter::toCerr(QString value)
{
  value += "\r\n";
  cerr << qPrintable(value);
}

QString EncoderParameter::getLevelForBitrate(int bitrate)
{
  if (bitrate > 240000000)
    return QObject::tr("auto");
  if (bitrate > 135000000)
    return QObject::tr("5.1");
  if (bitrate > 50000000)
    return QObject::tr("5.0");
  if (bitrate > 20000000)
    return QObject::tr("4.1");
  if (bitrate > 14000000)
    return QObject::tr("3.2");
  if (bitrate > 10000000)
    return QObject::tr("3.1");
  if (bitrate > 4000000)
    return QObject::tr("3.0");
  if (bitrate > 2000000)
    return QObject::tr("2.1");
  if (bitrate > 768000)
    return QObject::tr("2.0");
  if (bitrate > 384000)
    return QObject::tr("1.3");
  if (bitrate > 192000)
    return QObject::tr("1.2");
  if (bitrate > 128000)
    return QObject::tr("1.1");
  if (bitrate > 64000)
    return QObject::tr("1.0");
  return QObject::tr("auto");
}

QString EncoderParameter::getCurrentLevel()
{
  int profileLevel = this->getProfileLevel();
  if (profileLevel >= 65280) {
    return QObject::tr("auto");
  }
  if (profileLevel >= 13056) {
    return QObject::tr("5.1");
  }
  if (profileLevel >= 12800) {
    return QObject::tr("5.0");
  }
  if (profileLevel >= 10752) {
    return QObject::tr("4.2");
  }
  if (profileLevel >= 10496) {
    return QObject::tr("4.1");
  }
  if (profileLevel >= 10240) {
    return QObject::tr("4.0");
  }
  if (profileLevel >= 8192) {
    return QObject::tr("3.2");
  }
  if (profileLevel >= 7936) {
    return QObject::tr("3.1");
  }
  if (profileLevel >= 7680) {
    return QObject::tr("3.0");
  }
  if (profileLevel >= 5632) {
    return QObject::tr("2.2");
  }
  if (profileLevel >= 5376) {
    return QObject::tr("2.1");
  }
  if (profileLevel >= 5120) {
    return QObject::tr("2.0");
  }
  if (profileLevel >= 3328) {
    return QObject::tr("1.3");
  }
  if (profileLevel >= 3072) {
    return QObject::tr("1.2");
  }
  if (profileLevel >= 2816) {
    return QObject::tr("1.1");
  }
  if (profileLevel >= 2560) {
    return QObject::tr("1.0");
  }
  return QObject::tr("auto");
}

bool EncoderParameter::levelAsmallerB(QString a, QString b)
{
  QStringList levels;
  levels << QObject::tr("auto") << QObject::tr("5.1") << QObject::tr("5.0");
  levels << QObject::tr("4.2") << QObject::tr("4.1") << QObject::tr("4.0");
  levels << QObject::tr("3.2") << QObject::tr("3.1") << QObject::tr("3.0");
  levels << QObject::tr("2.2") << QObject::tr("2.1") << QObject::tr("2.0");
  levels << QObject::tr("1.3") << QObject::tr("1.2") << QObject::tr("1.1");
  levels << QObject::tr("1.0");
  QString min;
  while (!levels.isEmpty()) {
    min = levels.takeFirst();
    if (a == min) {
      return false;
    } else if (b == min) {
      return true;
    }
  }
  return true;
}

QString EncoderParameter::getCurrentProfile()
{
  QString profileLevel = QString::number(this->getProfileLevel());
  if (profileLevel.endsWith("6")) {
    return "baseline";
  }
  if (profileLevel.endsWith("7")) {
    return "main";
  }
  return "high";
}

bool EncoderParameter::checkSettings()
{
  //check if input, resolution and output are set at all
  if (this->getInputFileName().isEmpty()) {
    toCerr(QObject::tr("ERROR: No input specified!"));
    return false;
  }
  if (this->getInputSizeWidth() <= 0 || this->getInputSizeHeight() <= 0) {
    toCerr(QObject::tr("ERROR: No input width and height need to be specified!"));
    return false;
  }

  int bitrate = this->getAvgBitrate();
  int peakBitrate = this->getPeakBitrate();
  // check bitrate <= peak bitrate
  if (bitrate > peakBitrate) {
    toCerr(
        QObject::tr("ERROR: Target bitrate (%1) needs to be below peak bitrate (%2)!").arg(bitrate)
            .arg(peakBitrate));
    return false;
  }
  // check level for bitrate
  QString minLevelForBitrate = this->getLevelForBitrate(peakBitrate);
  QString currentLevel = this->getCurrentLevel();
  if (currentLevel != QObject::tr("auto") && minLevelForBitrate != currentLevel
      && !this->levelAsmallerB(minLevelForBitrate, currentLevel)) {
    toCerr(
        QObject::tr("ERROR: Current bitrate implies level %1, but current level is set to %2!").arg(
            minLevelForBitrate).arg(currentLevel));
    return false;
  }
  // check level for resolution
  QString minLevelForResolution = this->getMinLevelForFrameSize();
  if (currentLevel != QObject::tr("auto") && minLevelForResolution != currentLevel
      && !this->levelAsmallerB(minLevelForResolution, currentLevel)) {
    toCerr(QObject::tr("ERROR: Current resolution implies level %1, but current level is set to %2!").arg(minLevelForBitrate).arg(currentLevel));
    return false;
  }
  //no interlacing in baseline profile
  QString currentProfile = this->getCurrentProfile();
  cerr << "fieldMode: " << this->getFieldMode();
  cerr << "dieMode: " << this->getDiMode();
  cerr << "currentProfile: " << qPrintable(currentProfile);
  if (this->getFieldMode() != MODE_FRAME && this->getDiMode() != DI_MEDIAN
      && currentProfile == "baseline") {
    toCerr(QObject::tr("ERROR: Interlaced encoding is not allowed for baseline profile!"));
    return false;
  }

  return true;
}

int macroblocks(int dimension)
{
  int blocks = dimension / 16;
  if (dimension % 16 != 0)
    ++blocks;
  return blocks;
}

QString EncoderParameter::getMinLevelForFrameSize()
{

  double size = (double) macroblocks(this->getInputSizeWidth())
      * (double) macroblocks(this->getInputSizeHeight());
  if (size <= 99)
    return QObject::tr("1.0");
  if (size <= 396)
    return QObject::tr("1.1");
  if (size <= 792)
    return QObject::tr("2.1");
  if (size <= 1620)
    return QObject::tr("3.0");
  if (size <= 3600)
    return QObject::tr("3.1");
  if (size <= 5120)
    return QObject::tr("3.2");
  if (size <= 8192)
    return QObject::tr("4.0");
  if (size <= 22080)
    return QObject::tr("5.0");
  if (size <= 36864)
    return QObject::tr("5.1");
  return QObject::tr("auto");
}

bool EncoderParameter::setEncoderParameter(int argc, char *argv[])
{
  QString param, value;
  int profile = 0, level = 0;
  for (int i = 1; i < argc; ++i) {
    param = argv[i];
    param = param.trimmed();
    i += 1;
    if (i == argc) {
      toCerr(QObject::tr("ERROR: '%1' but no value!"));
      return false;
    }
    value = argv[i];
    EncoderHandler::removeQuotes(value);

    if (param == "--format") { //NVVE_SurfaceFormat
      toCerr(QObject::tr("SETTING colorformat %1: %2").arg(param).arg(value));
      if (value == "UYVY") {
        this->setColorFormat(UYVY);
      } else if (value == "YUY2") {
        this->setColorFormat(YUY2);
      } else if (value == "YV12") {
        this->setColorFormat(YV12);
      } else if (value == "NV12") {
        this->setColorFormat(NV12);
      } else if (value == "IYUV") {
        this->setColorFormat(IYUV);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--resolution") { //NVVE_IN/OUT_SIZE
      if (!value.contains("x")) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setResolution(value);
      continue;
    }

    if (param == "--input") {
      if (value != "-" && !QFile::exists(value)) {
        toCerr(QObject::tr("ERROR: Input file %1 doesn't exist!").arg(value));
        return false;
      }
      EncoderHandler::removeQuotes(value);
      this->setInputFileName(value);
      continue;
    }
    if (param == "--output") {
      EncoderHandler::removeQuotes(value);
      this->setOutputFileName(value);
      continue;
    }

    if (param == "--measure") {
      if (value == "PSNR") {
        this->setMeassure_fps(false);
        this->setMeassure_psnr(true);
      } else if (value == "FPS") {
        this->setMeassure_fps(true);
        this->setMeassure_psnr(false);
      } else if (value == "FPS_PSNR") {
        this->setMeassure_fps(true);
        this->setMeassure_psnr(true);
      } else if (value == "None") {
        this->setMeassure_fps(false);
        this->setMeassure_psnr(false);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--showFrameStats") {
      this->setShowFramestats(value.toInt());
      continue;
    }

    if (param == "--sar") {
      if (value == "1") {
        this->setAspectRatioWidth(1);
        this->setAspectRatioHeight(1);
        this->setAspectRatioType(ASPECT_RATIO_SAR);
        continue;
      }
      QStringList sar = value.split("x");
      if (sar.count() != 2) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setAspectRatioWidth(sar[0].toInt());
      this->setAspectRatioHeight(sar[1].toInt());
      this->setAspectRatioType(ASPECT_RATIO_SAR);
      continue;
    }
    if (param == "--control_mode") { //NVVE_RC_TYPE:
      if (value == "cbr") {
        this->setRCType(RC_CBR);
      } else if (value == "vbr") {
        this->setRCType(RC_VBR);
      } else if (value == "cq") {
        this->setRCType(RC_CQP);
      } else if (value == "vbr_rest") {
        this->setRCType(RC_VBR_MINQP);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }
    if (param == "--bitrate") { //NVVE_AVG_BITRATE
      int iValue = value.toInt() * 1000;
      if (iValue < 1000 || iValue > 300000000) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setAvgBitrate(iValue);
      continue;
    }
    if (param == "--bitrate_peak") { //NVVE_PEAK_BITRATE
      int iValue = value.toInt() * 1000;
      if (iValue < 1000 || iValue > 300000000) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setPeakBitrate(iValue);
      continue;
    }
    if (param == "--fps") { //NVVE_FRAME_RATE
      this->setFramerate(value);
      continue;
    }
    if (param == "--profile") { //NVVE_PROFILE_LEVEL:
      if (value == "baseline") {
        profile = 66;
      } else if (value == "main") {
        profile = 77;
      } else if (value == "high") {
        profile = 100;
      } else { //auto
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--level") {
      if (value == "auto") {
        level = 65280;
      } else if (value == "10") {
        level = 2560;
      } else if (value == "11") {
        level = 2816;
      } else if (value == "12") {
        level = 3072;
      } else if (value == "13") {
        level = 3328;
      } else if (value == "20") {
        level = 5120;
      } else if (value == "21") {
        level = 5376;
      } else if (value == "22") {
        level = 5632;
      } else if (value == "30") {
        level = 7680;
      } else if (value == "31") {
        level = 7936;
      } else if (value == "32") {
        level = 8192;
      } else if (value == "40") {
        level = 10240;
      } else if (value == "41") {
        level = 10496;
      } else if (value == "42") {
        level = 10752;
      } else if (value == "50") {
        level = 12800;
      } else if (value == "51") {
        level = 13056;
      } else {
        cerr
            << qPrintable(QObject::tr("ERROR: H264 - Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--offload") {
      if (value == "partial") {
        this->setGpuOffload(NVVE_GPU_OFFLOAD_ESTIMATORS);
      } else if (value == "full") {
        this->setGpuOffload(NVVE_GPU_OFFLOAD_ALL);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }
    if (param == "--forceGPU") {
      int iValue = value.toInt();
      if (iValue < 0) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setForcedGPU(iValue);
      continue;
    }

    if (param == "--frame_typ") { //NVVE_FIELD_ENC_MODE:
      if (value == "frame") {
        this->setFieldMode(MODE_FRAME);
      } else if (value == "top") {
        this->setFieldMode(MODE_FIELD_TOP_FIRST);
      } else if (value == "bottom") {
        this->setFieldMode(MODE_FIELD_BOTTOM_FIRST);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--pframe_dist") { //NVVE_P_INTERVAL:
      int iValue = value.toInt();
      if (iValue < 1 || iValue > 17) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setP_Interval(iValue);
      continue;
    }
    if (param == "--gop_max") { //NVVE_IDR_PERIODE
      int iValue = value.toInt();
      if (iValue < 1 || iValue > 10000) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setIDR_Period(iValue);
      continue;
    }

    if (param == "--dynamicGOP") { //NVVE_DYNAMIC_GOP
      if (value == "true") {
        this->setDynamicGOP(1);
      } else if (value == "false") {
        this->setDynamicGOP(0);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--pquant_min") { //NVVE_QP_LEVEL_INTER_P
      int iValue = value.toInt();
      if (iValue < 0 || iValue > 51) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setQP_Level_InterP(iValue);
      continue;
    }
    if (param == "--bquant_min") { //NVVE_QP_LEVEL_INTER_B
      int iValue = value.toInt();
      if (iValue < 0 || iValue > 51) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setQP_Level_InterB(iValue);
      continue;
    }
    if (param == "--iquant_min") { //NVVE_QP_LEVEL_INTRA
      int iValue = value.toInt();
      if (iValue < 0 || iValue > 51) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setQP_Level_Intra(iValue);
      continue;
    }

    if (param == "--deblock") { //NVVE_DEBLOCK_MODE
      if (value == "false") {
        this->setDeblockMode(0);
      } else if (value == "true") {
        this->setDeblockMode(1);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--deinterlace") { //NVVE_SET_DEINTERLACE:
      if (value == "false") {
        this->setDiMode(DI_OFF);
      } else if (value == "true") {
        this->setDiMode(DI_MEDIAN);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--preset") { //NVVE_PRESETS
      if (value == "psp") {
        this->setPresetsTarget(ENC_PRESET_PSP);
      } else if (value == "ipod") {
        this->setPresetsTarget(ENC_PRESET_IPOD);
      } else if (value == "avchd") {
        this->setPresetsTarget(ENC_PRESET_AVCHD);
      } else if (value == "bluray") {
        this->setPresetsTarget(ENC_PRESET_BD);
      } else if (value == "hdv1440") {
        this->setPresetsTarget(ENC_PRESET_HDV_1440);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--cavlc") { //NVVE_DISABLE_CABAC
      if (value == "false") {
        this->setDisableCabac(0);
      } else if (value == "true") {
        this->setDisableCabac(1);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--nal_typ") { //NVVE_CONFIGURE_NALU_FRAMING_TYPE:
      int iValue = value.toInt();
      if (iValue < 0 || iValue > 4) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setConfigNaluFramingType(iValue);
      continue;
    }

    if (param == "--sps_pps") { //NVVE_DISABLE_SPS_PPS
      if (value == "false") {
        this->setDisableSPSPPS(0);
      } else if (value == "true") {
        this->setDisableSPSPPS(1);
      } else {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      continue;
    }

    if (param == "--slices") { //NVVE_SLICE_COUNT
      if (value == "auto") {
        this->setSliceCount(0);
        continue;
      }
      int iValue = value.toInt();
      if (iValue < 1 || iValue > 4) {
        toCerr(QObject::tr("ERROR: Unsupported value for %1: %2").arg(param).arg(value));
        return false;
      }
      this->setSliceCount(iValue);
      continue;
    }

    toCerr(QObject::tr("ERROR: Unsupported %1: %2").arg(param).arg(value));
    return false;
  }

  //SET PROFILE AND LEVEL
  if (level != 0) { //Adjust Level
    int value = this->getProfileLevel();
    this->setProfileLevel(value - 65280 + level);
  }
  if (profile != 0) { //Adjust Profile
    int value = this->getProfileLevel();
    this->setProfileLevel(value - 100 + profile);
  }

  if (!this->checkSettings()) {
    return false;
  }

  return true;
}

void EncoderParameter::setDefaults()
{
  this->inputFileName = "-"; //PIPE INPUT
  this->iCodecType = 4; //4->H.264, 5 = VC-1
  //NVVE_ASPECT_RATIO
  this->iAspectRatioWidth = 1;
  this->iAspectRatioHeight = 1;
  this->aspectRatioType = ASPECT_RATIO_SAR; //NVVE_ASPECT_RATIO_TYPE
  //NVVE_RC_TYPE
  this->RCType = RC_VBR_MINQP;
  //NVVE_AVG_BITRATE
  this->iAvgBitrate = 1500000; //bit/s
  //NVVE_PEAK_BITRATE
  this->iPeakBitrate = 300000000; //bit/s
  //NVVE_FRAME_RATE
  this->iFrameRateNumerator = 25;
  this->iFrameRateDenominator = 1;
  //NVVE_PROFILE_LEVEL
  this->iProfileLevel = 65380; //ff:auto 0x64:High -> ff64 = 65380
  //NVVE_FIELD_ENC_MODE
  this->fieldMode = MODE_FRAME;
  //NVVE_P_INTERVAL
  this->iP_Interval = 1;
  //NVVE_IDR_PERIOD
  this->iIDR_Period = 250;
  //NVVE_DYNAMIC_GOP
  this->iDynamicGOP = 1;
  //NVVE_QP_LEVEL_INTER_P
  this->iQP_Level_InterP = 12;
  //NVVE_QP_LEVEL_INTER_B
  this->iQP_Level_InterB = 15;
  //NVVE_QP_LEVEL_INTRA
  this->iQP_Level_Intra = 10;
  //NVVE_DEBLOCK_MODE
  this->iDeblockMode = 1;
  //NVVE_SET_DEINTERLACE:
  this->diMode = DI_OFF;
  //NVVE_PRESETS
  this->presetsTarget = -1;
  //NVVE_DISABLE_CABAC
  this->iDisableCabac = 0;
  //NVVE_CONFIGURE_NALU_FRAMING_TYPE
  this->iConfigNaluFramingType = 0;
  //NVVE_DISABLE_SPS_PPS
  this->iDisableSPSPPS = 0;
  //NVVE_SLICE_COUNT
  this->iSliceCount = 0;
  //NVVE_GPU_OFFLOAD
  this->gpuOffload = NVVE_GPU_OFFLOAD_ESTIMATORS;
  //FORCE A SPECIFIC GPU
  this->forceDevice = 0;
  this->iForcedGPU = 1;
  this->measure_fps = true;
  this->measure_psnr = false;
  this->iInputSizeHeight = 0;
  this->iInputSizeWidth = 0;
  this->colorFormat = IYUV;
  this->showFramestats = 0;
}

int EncoderParameter::getShowFramestats() const
{
  return showFramestats;
}

void EncoderParameter::setShowFramestats(int showFramestats)
{
  this->showFramestats = showFramestats;
}

NVVE_SurfaceFormat EncoderParameter::getColorFormat() const
{
  return colorFormat;
}

void EncoderParameter::setColorFormat(NVVE_SurfaceFormat colorFormat)
{
  this->colorFormat = colorFormat;
}

NVVE_ASPECT_RATIO_TYPE EncoderParameter::getAspectRatioType() const
{
  return aspectRatioType;
}

int EncoderParameter::getAspectRatioWidth() const
{
  return iAspectRatioWidth;
}

int EncoderParameter::getAspectRatioHeight() const
{
  return iAspectRatioHeight;
}

int EncoderParameter::getAvgBitrate() const
{
  return iAvgBitrate;
}

int EncoderParameter::getCPU_count() const
{
  return iCPU_count;
}

int EncoderParameter::getCodecType() const
{
  return iCodecType;
}

int EncoderParameter::getConfigNaluFramingType() const
{
  return iConfigNaluFramingType;
}

int EncoderParameter::getDeblockMode() const
{
  return iDeblockMode;
}

NVVE_DI_MODE EncoderParameter::getDiMode() const
{
  return diMode;
}

int EncoderParameter::getDisableCabac() const
{
  return iDisableCabac;
}

int EncoderParameter::getDisableSPSPPS() const
{
  return iDisableSPSPPS;
}

int EncoderParameter::getDynamicGOP() const
{
  return iDynamicGOP;
}

NVVE_FIELD_MODE EncoderParameter::getFieldMode() const
{
  return fieldMode;
}

int EncoderParameter::getForcedGPU() const
{
  return iForcedGPU;
}

int EncoderParameter::getFrameRateDenominator() const
{
  return iFrameRateDenominator;
}

int EncoderParameter::getFrameRateNumerator() const
{
  return iFrameRateNumerator;
}

int EncoderParameter::getGPU_devID() const
{
  return iGPU_devID;
}

NVVE_GPUOffloadLevel EncoderParameter::getGpuOffload() const
{
  return gpuOffload;
}

int EncoderParameter::getIDR_Period() const
{
  return iIDR_Period;
}

QString EncoderParameter::getInputFileName() const
{
  return inputFileName;
}

int EncoderParameter::getInputSizeHeight() const
{
  return iInputSizeHeight;
}

int EncoderParameter::getInputSizeWidth() const
{
  return iInputSizeWidth;
}

bool EncoderParameter::getMeasure_fps() const
{
  return measure_fps;
}

bool EncoderParameter::getMeasure_psnr() const
{
  return measure_psnr;
}

QString EncoderParameter::getOutputFileName() const
{
  return outputFileName;
}

int EncoderParameter::getP_Interval() const
{
  return iP_Interval;
}

int EncoderParameter::getPeakBitrate() const
{
  return iPeakBitrate;
}

int EncoderParameter::getPresetsTarget() const
{
  return presetsTarget;
}

int EncoderParameter::getProfileLevel() const
{
  return iProfileLevel;
}

int EncoderParameter::getQP_Level_InterB() const
{
  return iQP_Level_InterB;
}

int EncoderParameter::getQP_Level_InterP() const
{
  return iQP_Level_InterP;
}

int EncoderParameter::getQP_Level_Intra() const
{
  return iQP_Level_Intra;
}

NVVE_RateCtrlType EncoderParameter::getRCType() const
{
  return RCType;
}

int EncoderParameter::getSliceCount() const
{
  return iSliceCount;
}

int EncoderParameter::getGPUCount() const
{
  return this->iGPUCount;
}

int EncoderParameter::getForceDevice() const
{
  return this->forceDevice;
}

void EncoderParameter::setForceDevice(int forceDevice)
{
  this->forceDevice = forceDevice;
}

void EncoderParameter::setGPUCount(int iGPUCount)
{
  this->iGPUCount = iGPUCount;
}
void EncoderParameter::setAspectRatioHeight(int iAspectRatioHeight)
{
  this->iAspectRatioHeight = iAspectRatioHeight;
}

void EncoderParameter::setAspectRatioType(NVVE_ASPECT_RATIO_TYPE aspectRatioType)
{
  this->aspectRatioType = aspectRatioType;
}

void EncoderParameter::setAspectRatioWidth(int iAspectRatioWidth)
{
  this->iAspectRatioWidth = iAspectRatioWidth;
}

void EncoderParameter::setAvgBitrate(int iAvgBitrate)
{
  this->iAvgBitrate = iAvgBitrate;
}

void EncoderParameter::setCPU_count(int iCPU_count)
{
  this->iCPU_count = iCPU_count;
}

void EncoderParameter::setCodecType(int iCodecType)
{
  this->iCodecType = iCodecType;
}

void EncoderParameter::setConfigNaluFramingType(int iConfigNaluFramingType)
{
  this->iConfigNaluFramingType = iConfigNaluFramingType;
}

void EncoderParameter::setDeblockMode(int iDeblockMode)
{
  this->iDeblockMode = iDeblockMode;
}

void EncoderParameter::setDiMode(NVVE_DI_MODE diMode)
{
  this->diMode = diMode;
}

void EncoderParameter::setDisableCabac(int iDisableCabac)
{
  this->iDisableCabac = iDisableCabac;
}

void EncoderParameter::setDisableSPSPPS(int iDisableSPSPPS)
{
  this->iDisableSPSPPS = iDisableSPSPPS;
}

void EncoderParameter::setDynamicGOP(int iDynamicGOP)
{
  this->iDynamicGOP = iDynamicGOP;
}

void EncoderParameter::setFieldMode(NVVE_FIELD_MODE fieldMode)
{
  this->fieldMode = fieldMode;
}

void EncoderParameter::setForcedGPU(int iForcedGPU)
{
  this->forceDevice = 1;
  this->iForcedGPU = iForcedGPU;
}

void EncoderParameter::setFramerate(const QString framerate)
{
  int numerator, denominator;

  double value = framerate.toDouble();
  if (framerate.contains("/")) {
    QStringList values = framerate.split("/");
    this->iFrameRateNumerator = values.at(0).toInt();
    this->iFrameRateDenominator = values.at(1).toInt();
    return;
  } else if (int(value) - value == 0) {
    numerator = framerate.toInt();
    denominator = 1;
  } else if (framerate.startsWith("23.97")) {
    numerator = 24000;
    denominator = 1001;
  } else if (framerate.startsWith("29.97")) {
    numerator = 30000;
    denominator = 1001;
  } else if (framerate.startsWith("59.94")) {
    numerator = 60000;
    denominator = 1001;
  } else if (framerate.startsWith("11.98")) {
    numerator = 12000;
    denominator = 1001;
  } else if (framerate.startsWith("119.88")) {
    numerator = 120000;
    denominator = 1001;
  } else if (value == 12.5) {
    numerator = 12;
    denominator = 2;
  } else if (framerate.startsWith("14.98")) {
    numerator = 15000;
    denominator = 1001;
  } else {
    numerator = int(value * 1000);
    denominator = 1000;
  }
  this->iFrameRateNumerator = numerator;
  this->iFrameRateDenominator = denominator;
}

void EncoderParameter::setFrameRateDenominator(int iFrameRateDenominator)
{
  this->iFrameRateDenominator = iFrameRateDenominator;
}

void EncoderParameter::setFrameRateNumerator(int iFrameRateNumerator)
{
  this->iFrameRateNumerator = iFrameRateNumerator;
}

void EncoderParameter::setGPU_devID(int iGPU_devID)
{
  this->iGPU_devID = iGPU_devID;
}

void EncoderParameter::setGpuOffload(NVVE_GPUOffloadLevel gpuOffload)
{
  this->gpuOffload = gpuOffload;
}

void EncoderParameter::setIDR_Period(int iIDR_Period)
{
  this->iIDR_Period = iIDR_Period;
}

void EncoderParameter::setInputFileName(QString inputFileName)
{
  this->inputFileName = inputFileName;
}

void EncoderParameter::setResolution(QString resolution)
{
  QStringList res = resolution.split("x");
  this->iInputSizeWidth = res[0].toInt();
  this->iInputSizeHeight = res[1].toInt();
}

void EncoderParameter::setInputSizeHeight(int iInputSizeHeight)
{
  this->iInputSizeHeight = iInputSizeHeight;
}

void EncoderParameter::setInputSizeWidth(int iInputSizeWidth)
{
  this->iInputSizeWidth = iInputSizeWidth;
}

void EncoderParameter::setMeassure_fps(bool measure_fps)
{
  this->measure_fps = measure_fps;
}

void EncoderParameter::setMeassure_psnr(bool measure_psnr)
{
  this->measure_psnr = measure_psnr;
}

void EncoderParameter::setOutputFileName(QString outputFileName)
{
  this->outputFileName = outputFileName;
}

void EncoderParameter::setP_Interval(int iP_Interval)
{
  this->iP_Interval = iP_Interval;
}

void EncoderParameter::setPeakBitrate(int iPeakBitrate)
{
  this->iPeakBitrate = iPeakBitrate;
}

void EncoderParameter::setPresetsTarget(NVVE_PRESETS_TARGET presetsTarget)
{
  this->presetsTarget = (int) presetsTarget;
}

void EncoderParameter::setProfileLevel(int iProfileLevel)
{
  this->iProfileLevel = iProfileLevel;
}

void EncoderParameter::setQP_Level_InterB(int iQP_Level_InterB)
{
  this->iQP_Level_InterB = iQP_Level_InterB;
}

void EncoderParameter::setQP_Level_InterP(int iQP_Level_InterP)
{
  this->iQP_Level_InterP = iQP_Level_InterP;
}

void EncoderParameter::setQP_Level_Intra(int iQP_Level_Intra)
{
  this->iQP_Level_Intra = iQP_Level_Intra;
}

void EncoderParameter::setRCType(NVVE_RateCtrlType RCType)
{
  this->RCType = RCType;
}

void EncoderParameter::setSliceCount(int iSliceCount)
{
  this->iSliceCount = iSliceCount;
}
