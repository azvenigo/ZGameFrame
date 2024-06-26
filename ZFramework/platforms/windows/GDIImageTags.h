#pragma once
#include <map>
#include <string>
#include "helpers/StringHelpers.h"

namespace GDIHelpers
{

    std::map<std::string, size_t> gdiTagTable = {
     {"ExifIFD",0x8769}
    ,{"GpsIFD",0x8825}
    ,{"NewSubfileType",0x00FE}
    ,{"SubfileType",0x00FF}
    ,{"ImageWidth",0x0100}
    ,{"ImageHeight",0x0101}
    ,{"BitsPerSample",0x0102}
    ,{"Compression",0x0103}
    ,{"PhotometricInterp",0x0106}
    ,{"ThreshHolding",0x0107}
    ,{"CellWidth",0x0108}
    ,{"CellHeight",0x0109}
    ,{"FillOrder",0x010A}
    ,{"DocumentName",0x010D}
    ,{"ImageDescription",0x010E}
    ,{"EquipMake",0x010F}
    ,{"EquipModel",0x0110}
    ,{"StripOffsets",0x0111}
    ,{"Orientation",0x0112}
    ,{"SamplesPerPixel",0x0115}
    ,{"RowsPerStrip",0x0116}
    ,{"StripBytesCount",0x0117}
    ,{"MinSampleValue",0x0118}
    ,{"MaxSampleValue",0x0119}
    ,{"XResolution",0x011A   }// Image resolution in width direction
    ,{"YResolution",0x011B   }// Image resolution in height direction
    ,{"PlanarConfig",0x011C   }// Image data arrangement
    ,{"PageName",0x011D}
    ,{"XPosition",0x011E}
    ,{"YPosition",0x011F}
    ,{"FreeOffset",0x0120}
    ,{"FreeByteCounts",0x0121}
    ,{"GrayResponseUnit",0x0122}
    ,{"GrayResponseCurve",0x0123}
    ,{"T4Option",0x0124}
    ,{"T6Option",0x0125}
    ,{"ResolutionUnit",0x0128   }// Unit of X and Y resolution
    ,{"PageNumber",0x0129}
    ,{"TransferFuncition",0x012D}
    ,{"SoftwareUsed",0x0131}
    ,{"DateTime",0x0132}
    ,{"Artist",0x013B}
    ,{"HostComputer",0x013C}
    ,{"Predictor",0x013D}
    ,{"WhitePoint",0x013E}
    ,{"PrimaryChromaticities",0x013F}
    ,{"ColorMap",0x0140}
    ,{"HalftoneHints",0x0141}
    ,{"TileWidth",0x0142}
    ,{"TileLength",0x0143}
    ,{"TileOffset",0x0144}
    ,{"TileByteCounts",0x0145}
    ,{"InkSet",0x014C}
    ,{"InkNames",0x014D}
    ,{"NumberOfInks",0x014E}
    ,{"DotRange",0x0150}
    ,{"TargetPrinter",0x0151}
    ,{"ExtraSamples",0x0152}
    ,{"SampleFormat",0x0153}
    ,{"SMinSampleValue",0x0154}
    ,{"SMaxSampleValue",0x0155}
    ,{"TransferRange",0x0156}
    ,{"JPEGProc",0x0200}
    ,{"JPEGInterFormat",0x0201}
    ,{"JPEGInterLength",0x0202}
    ,{"JPEGRestartInterval",0x0203}
    ,{"JPEGLosslessPredictors",0x0205}
    ,{"JPEGPointTransforms",0x0206}
    ,{"JPEGQTables",0x0207}
    ,{"JPEGDCTables",0x0208}
    ,{"JPEGACTables",0x0209}
    ,{"YCbCrCoefficients",0x0211}
    ,{"YCbCrSubsampling",0x0212}
    ,{"YCbCrPositioning",0x0213}
    ,{"REFBlackWhite",0x0214}
    ,{"ICCProfile",0x8773   }// This TAG is defined by ICC
    ,{"Gamma",0x0301}
    ,{"ICCProfileDescriptor",0x0302}
    ,{"SRGBRenderingIntent",0x0303}
    ,{"ImageTitle",0x0320}
    ,{"Copyright",0x8298}
    ,{"ResolutionXUnit",0x5001}
    ,{"ResolutionYUnit",0x5002}
    ,{"ResolutionXLengthUnit",0x5003}
    ,{"ResolutionYLengthUnit",0x5004}
    ,{"PrintFlags",0x5005}
    ,{"PrintFlagsVersion",0x5006}
    ,{"PrintFlagsCrop",0x5007}
    ,{"PrintFlagsBleedWidth",0x5008}
    ,{"PrintFlagsBleedWidthScale",0x5009}
    ,{"HalftoneLPI",0x500A}
    ,{"HalftoneLPIUnit",0x500B}
    ,{"HalftoneDegree",0x500C}
    ,{"HalftoneShape",0x500D}
    ,{"HalftoneMisc",0x500E}
    ,{"HalftoneScreen",0x500F}
    ,{"JPEGQuality",0x5010}
    ,{"GridSize",0x5011}
    ,{"ThumbnailFormat",0x5012  }// 1 = JPEG, 0 = RAW RGB
    ,{"ThumbnailWidth",0x5013}
    ,{"ThumbnailHeight",0x5014}
    ,{"ThumbnailColorDepth",0x5015}
    ,{"ThumbnailPlanes",0x5016}
    ,{"ThumbnailRawBytes",0x5017}
    ,{"ThumbnailSize",0x5018}
    ,{"ThumbnailCompressedSize",0x5019}
    ,{"ColorTransferFunction",0x501A}
    ,{"ThumbnailData",0x501B}// RAW thumbnail bits in
    ,{"ThumbnailImageWidth",0x5020  }// Thumbnail width
    ,{"ThumbnailImageHeight",0x5021  }// Thumbnail height
    ,{"ThumbnailBitsPerSample",0x5022  }// Number of bits per
    ,{"ThumbnailCompression",0x5023  }// Compression Scheme
    ,{"ThumbnailPhotometricInterp",0x5024 }// Pixel composition
    ,{"ThumbnailImageDescription",0x5025  }// Image Tile
    ,{"ThumbnailEquipMake",0x5026  }// Manufacturer of Image
    ,{"ThumbnailEquipModel",0x5027  }// Model of Image input
    ,{"ThumbnailStripOffsets",0x5028  }// Image data location
    ,{"ThumbnailOrientation",0x5029  }// Orientation of image
    ,{"ThumbnailSamplesPerPixel",0x502A  }// Number of components
    ,{"ThumbnailRowsPerStrip",0x502B  }// Number of rows per strip
    ,{"ThumbnailStripBytesCount",0x502C  }// Bytes per compressed
    ,{"ThumbnailResolutionX",0x502D  }// Resolution in width
    ,{"ThumbnailResolutionY",0x502E  }// Resolution in height
    ,{"ThumbnailPlanarConfig",0x502F  }// Image data arrangement
    ,{"ThumbnailResolutionUnit",0x5030  }// Unit of X and Y
    ,{"ThumbnailTransferFunction",0x5031  }// Transfer function
    ,{"ThumbnailSoftwareUsed",0x5032  }// Software used
    ,{"ThumbnailDateTime",0x5033  }// File change date and
    ,{"ThumbnailArtist",0x5034  }// Person who created the
    ,{"ThumbnailWhitePoint",0x5035  }// White point chromaticity
    ,{"ThumbnailPrimaryChromaticities",0x5036 }
    ,{"ThumbnailYCbCrCoefficients",0x5037 }// Color space transforma-
    ,{"ThumbnailYCbCrSubsampling",0x5038  }// Subsampling ratio of Y
    ,{"ThumbnailYCbCrPositioning",0x5039  }// Y and C position
    ,{"ThumbnailRefBlackWhite",0x503A  }// Pair of black and white
    ,{"ThumbnailCopyRight",0x503B  }// CopyRight holder
    ,{"LuminanceTable",0x5090}
    ,{"ChrominanceTable",0x5091}
    ,{"FrameDelay",0x5100}
    ,{"LoopCount",0x5101}
    ,{"GlobalPalette",0x5102}
    ,{"IndexBackground",0x5103}
    ,{"IndexTransparent",0x5104}
    ,{"PixelUnit",0x5110  }// Unit specifier for pixel/unit
    ,{"PixelPerUnitX",0x5111  }// Pixels per unit in X
    ,{"PixelPerUnitY",0x5112  }// Pixels per unit in Y
    ,{"PaletteHistogram",0x5113  }// Palette histogram
    ,{"ExifExposureTime",0x829A}
    ,{"ExifFNumber",0x829D}
    ,{"ExifExposureProg",0x8822}
    ,{"ExifSpectralSense",0x8824}
    ,{"ExifISOSpeed",0x8827}
    ,{"ExifOECF",0x8828}
    ,{"ExifVer",0x9000}
    ,{"ExifDTOrig",0x9003 }// Date & time of original
    ,{"ExifDTDigitized",0x9004 }// Date & time of digital data generation
    ,{"ExifCompConfig",0x9101}
    ,{"ExifCompBPP",0x9102}
    ,{"ExifShutterSpeed",0x9201}
    ,{"ExifAperture",0x9202}
    ,{"ExifBrightness",0x9203}
    ,{"ExifExposureBias",0x9204}
    ,{"ExifMaxAperture",0x9205}
    ,{"ExifSubjectDist",0x9206}
    ,{"ExifMeteringMode",0x9207}
    ,{"ExifLightSource",0x9208}
    ,{"ExifFlash",0x9209}
    ,{"ExifFocalLength",0x920A}
    ,{"ExifSubjectArea",0x9214  }// exif 2.2 Subject Area
    ,{"ExifMakerNote",0x927C}
    ,{"ExifUserComment",0x9286}
    ,{"ExifDTSubsec",0x9290  }// Date & Time subseconds
    ,{"ExifDTOrigSS",0x9291  }// Date & Time original subseconds
    ,{"ExifDTDigSS",0x9292  }// Date & TIme digitized subseconds
    ,{"ExifFPXVer",0xA000}
    ,{"ExifColorSpace",0xA001}
    ,{"ExifPixXDim",0xA002}
    ,{"ExifPixYDim",0xA003}
    ,{"ExifRelatedWav",0xA004  }// related sound file
    ,{"ExifInterop",0xA005}
    ,{"ExifFlashEnergy",0xA20B}
    ,{"ExifSpatialFR",0xA20C  }// Spatial Frequency Response
    ,{"ExifFocalXRes",0xA20E  }// Focal Plane X Resolution
    ,{"ExifFocalYRes",0xA20F  }// Focal Plane Y Resolution
    ,{"ExifFocalResUnit",0xA210  }// Focal Plane Resolution Unit
    ,{"ExifSubjectLoc",0xA214}
    ,{"ExifExposureIndex",0xA215}
    ,{"ExifSensingMethod",0xA217}
    ,{"ExifFileSource",0xA300}
    ,{"ExifSceneType",0xA301}
    ,{"ExifCfaPattern",0xA302}
    ,{"ExifCustomRendered",0xA401}
    ,{"ExifExposureMode",0xA402}
    ,{"ExifWhiteBalance",0xA403}
    ,{"ExifDigitalZoomRatio",0xA404}
    ,{"ExifFocalLengthIn35mmFilm",0xA405}
    ,{"ExifSceneCaptureType",0xA406}
    ,{"ExifGainControl",0xA407}
    ,{"ExifContrast",0xA408}
    ,{"ExifSaturation",0xA409}
    ,{"ExifSharpness",0xA40A}
    ,{"ExifDeviceSettingDesc",0xA40B}
    ,{"ExifSubjectDistanceRange",0xA40C}
    ,{"ExifUniqueImageID",0xA420}
    ,{"GpsVer",0x0000}
    ,{"GpsLatitudeRef",0x0001}
    ,{"GpsLatitude",0x0002}
    ,{"GpsLongitudeRef",0x0003}
    ,{"GpsLongitude",0x0004}
    ,{"GpsAltitudeRef",0x0005}
    ,{"GpsAltitude",0x0006}
    ,{"GpsGpsTime",0x0007}
    ,{"GpsGpsSatellites",0x0008}
    ,{"GpsGpsStatus",0x0009}
    ,{"GpsGpsMeasureMode",0x00A}
    ,{"GpsGpsDop",0x000B  }// Measurement precision
    ,{"GpsSpeedRef",0x000C}
    ,{"GpsSpeed",0x000D}
    ,{"GpsTrackRef",0x000E}
    ,{"GpsTrack",0x000F}
    ,{"GpsImgDirRef",0x0010}
    ,{"GpsImgDir",0x0011}
    ,{"GpsMapDatum",0x0012}
    ,{"GpsDestLatRef",0x0013}
    ,{"GpsDestLat",0x0014}
    ,{"GpsDestLongRef",0x0015}
    ,{"GpsDestLong",0x0016}
    ,{"GpsDestBearRef",0x0017}
    ,{"GpsDestBear",0x0018}
    ,{"GpsDestDistRef",0x0019}
    ,{"GpsDestDist",0x001A}
    ,{"GpsProcessingMethod",0x001B}
    ,{"GpsAreaInformation",0x001C}
    ,{"GpsDate",0x001D}
    ,{"GpsDifferential",0x001E}
    };

    std::string TagFromID(size_t nID)
    {
        auto result = std::find_if(gdiTagTable.begin(), gdiTagTable.end(), [nID](const auto& mapItem) { return mapItem.second == nID; });
        if (result == gdiTagTable.end())
            return "unknown_tag";

        return result->first;
    }

    std::string TypeString(size_t nType)
    {
        switch (nType)
        {
        case PropertyTagTypeASCII:      return "TypeASCII";
        case PropertyTagTypeByte:       return "TypeByte";
        case PropertyTagTypeLong:       return "TypeLong";
        case PropertyTagTypeRational:   return "TypeRational";
        case PropertyTagTypeShort:      return "TypeShort";
        case PropertyTagTypeSLONG:      return "TypeSLONG";
        case PropertyTagTypeSRational:  return "TypeSRational";
        case PropertyTagTypeUndefined:  return "TypeUndefined";
        }

        return "unknown_type";
    }

    std::string ValueStringByType(size_t nType, void* pValue, int32_t nLength)
    {
        switch (nType)
        {
        case PropertyTagTypeASCII:         return std::string((const char*)pValue, nLength);
        case PropertyTagTypeByte:          return SH::FromBin((uint8_t*)pValue, nLength);
        case PropertyTagTypeLong:          return SH::FromBin((uint8_t*)pValue, nLength);
        case PropertyTagTypeRational:      return SH::FromBin((uint8_t*)pValue, nLength);
        case PropertyTagTypeShort:         return SH::FromBin((uint8_t*)pValue, nLength);
        case PropertyTagTypeSLONG:         return SH::FromBin((uint8_t*)pValue, nLength);
        case PropertyTagTypeSRational:     return SH::FromBin((uint8_t*)pValue, nLength);
        }
        return "";
    }

    std::string PropertyItemToString(PropertyItem* p)
    {
        std::string sType;
        std::string sValue;

        // cap the length returned
        int32_t nLength = p->length;
        if (nLength > 64)
            nLength = 64;

        std::string sResult;
        Sprintf(sResult, "id:%s (0x%08d) type:%s length:%d value:%s", TagFromID(p->id).c_str(), p->id, TypeString(p->type), p->length, ValueStringByType(p->type, p->value, nLength));
        return sResult;
    }
};