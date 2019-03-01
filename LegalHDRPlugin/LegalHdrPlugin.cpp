#include "LegalHdrPlugin.h"

#include <stdio.h>

// #include "../../openfx/Support/include/ofxsImageEffect.h"
// #include "../../openfx/Support/include/ofxsMultiThread.h"
// #include "../../openfx/Support/Plugins/include/ofxsProcessing.h"
// #include "../../openfx/Support/include/ofxsLog.h"

#include "../Support/include/ofxsImageEffect.h"
#include "../Support/include/ofxsMultiThread.h"
#include "../Support/include/ofxsProcessing.h"
#include "../Support/include/ofxsLog.h"


#define kPluginName "Highlight HDR"
#define kPluginGrouping "chrisfx"
#define kPluginDescription \
"Highlight pixels in PQ/ST2084 images that are brighter\n" \
"than a defined threshold, up to 10,000 nits. The highlight\n" \
"can be triggered by RGB values or Rec2020 luma."
#define kPluginIdentifier "com.chrisfx.samples.highlighthdr"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles false
#define kSupportsMultiResolution false
#define kSupportsMultipleClipPARs false

////////////////////////////////////////////////////////////////////////////////

class LegalHdr : public OFX::ImageProcessor
{
public:
    explicit LegalHdr(OFX::ImageEffect& p_Instance);

    virtual void processImagesCUDA();
    //virtual void processImagesOpenCL();
    //virtual void processImagesMetal();
    virtual void multiThreadProcessImages(OfxRectI p_ProcWindow);

    void setSrcImg(OFX::Image* p_SrcImg);
    void setScales(float p_Nits, float p_OverlayRgbColorR, float p_OverlayRgbColorG, float p_OverlayRgbColorB, int p_DisplayOverlay, int p_LumaOverlay);

private:
    OFX::Image* _srcImg;
    float _nits;
    float _overlayRgbColor[3];
    int _displayOverlay;
    int _lumaOverlay;
};

LegalHdr::LegalHdr(OFX::ImageEffect& p_Instance)
    : OFX::ImageProcessor(p_Instance)
{
}

extern void RunCudaKernel(int p_Width, int p_Height, float p_Luminance, float* p_OverlayRgb, int p_OverlayDisplay, int p_OverlayLuma, const float* p_Input, float* p_Output);

void LegalHdr::processImagesCUDA()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    RunCudaKernel(width, height, _nits, _overlayRgbColor, _displayOverlay, _lumaOverlay, input, output);
}

//#ifdef __APPLE__
//extern void RunMetalKernel(int p_Width, int p_Height, float* p_Gain, const float* p_Input, float* p_Output);
//#endif
//
//void LegalHdr::processImagesMetal()
//{
//#ifdef __APPLE__
//    const OfxRectI& bounds = _srcImg->getBounds();
//    const int width = bounds.x2 - bounds.x1;
//    const int height = bounds.y2 - bounds.y1;
//
//    float* input = static_cast<float*>(_srcImg->getPixelData());
//    float* output = static_cast<float*>(_dstImg->getPixelData());
//
//    RunMetalKernel(width, height, _scales, input, output);
//#endif
//}

//extern void RunOpenCLKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Gain, const float* p_Input, float* p_Output);
//
//void LegalHdr::processImagesOpenCL()
//{
//    const OfxRectI& bounds = _srcImg->getBounds();
//    const int width = bounds.x2 - bounds.x1;
//    const int height = bounds.y2 - bounds.y1;
//
//    float* input = static_cast<float*>(_srcImg->getPixelData());
//    float* output = static_cast<float*>(_dstImg->getPixelData());
//
//    RunOpenCLKernel(_pOpenCLCmdQ, width, height, _scales, input, output);
//}

void LegalHdr::multiThreadProcessImages(OfxRectI p_ProcWindow)
{
    for (int y = p_ProcWindow.y1; y < p_ProcWindow.y2; ++y)
    {
        if (_effect.abort()) break;

        float* dstPix = static_cast<float*>(_dstImg->getPixelAddress(p_ProcWindow.x1, y));

        for (int x = p_ProcWindow.x1; x < p_ProcWindow.x2; ++x)
        {
            float* srcPix = static_cast<float*>(_srcImg ? _srcImg->getPixelAddress(x, y) : 0);

            // do we have a source image to scale up
            if (srcPix)
            {
                for(int c = 0; c < 4; ++c)
                {
                    dstPix[c] = srcPix[c]; // * _scales[c];
                }
            }
            else
            {
                // no src pixel here, be black and transparent
                for (int c = 0; c < 4; ++c)
                {
                    dstPix[c] = 0;
                }
            }

            // increment the dst pixel
            dstPix += 4;
        }
    }
}

void LegalHdr::setSrcImg(OFX::Image* p_SrcImg)
{
    _srcImg = p_SrcImg;
}

void LegalHdr::setScales(float p_Nits, float p_OverlayRgbColorR, float p_OverlayRgbColorG, float p_OverlayRgbColorB, int p_DisplayOverlay, int p_LumaOverlay)
{
    _nits = p_Nits;
    _overlayRgbColor[0] = p_OverlayRgbColorR;
    _overlayRgbColor[1] = p_OverlayRgbColorG;
    _overlayRgbColor[2] = p_OverlayRgbColorB;
    _displayOverlay = p_DisplayOverlay;
    _lumaOverlay = p_LumaOverlay;
}


////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class LegalHdrPlugin : public OFX::ImageEffect
{
public:
    explicit LegalHdrPlugin(OfxImageEffectHandle p_Handle);

    /* Override the render */
    virtual void render(const OFX::RenderArguments& p_Args);

    /* Override is identity */
    virtual bool isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime);

    /* Override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName);

    /* Override changed clip */
    virtual void changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName);

    /* Set the enabledness of the component scale params depending on the type of input image and the state of the scaleComponents param */
    // void setEnabledness();

    /* Set up and run a processor */
    void setupAndProcess(LegalHdr &p_LegalHdr, const OFX::RenderArguments& p_Args);

private:
    // Does not own the following pointers
    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;

    OFX::DoubleParam* m_ThresholdNits;
    OFX::DoubleParam* m_OverlayRgbColorR;
    OFX::DoubleParam* m_OverlayRgbColorG;
    OFX::DoubleParam* m_OverlayRgbColorB;
    OFX::BooleanParam* m_DisplayOverlay;
    OFX::BooleanParam* m_LumaOverlay;
};

LegalHdrPlugin::LegalHdrPlugin(OfxImageEffectHandle p_Handle)
    : ImageEffect(p_Handle)
{
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);

    m_ThresholdNits = fetchDoubleParam("thresholdNits");
    m_OverlayRgbColorR = fetchDoubleParam("overlayR");
    m_OverlayRgbColorG = fetchDoubleParam("overlayG");
    m_OverlayRgbColorB = fetchDoubleParam("overlayB");
    m_DisplayOverlay = fetchBooleanParam("displayOverlay");
    m_LumaOverlay = fetchBooleanParam("lumaOverlay");

}

void LegalHdrPlugin::render(const OFX::RenderArguments& p_Args)
{
    if ((m_DstClip->getPixelDepth() == OFX::eBitDepthFloat) && (m_DstClip->getPixelComponents() == OFX::ePixelComponentRGBA))
    {
        LegalHdr legalHdr(*this);
        setupAndProcess(legalHdr, p_Args);
    }
    else
    {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
}

bool LegalHdrPlugin::isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime)
{
    if (!m_DisplayOverlay->getValueAtTime(p_Args.time))
    {
        p_IdentityClip = m_SrcClip;
        p_IdentityTime = p_Args.time;
        return true;
    }

    return false;
}

void LegalHdrPlugin::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName)
{
    if (p_ParamName == "displayOverlay")
    {
        // setEnabledness();
    }
}

void LegalHdrPlugin::changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName)
{
    if (p_ClipName == kOfxImageEffectSimpleSourceClipName)
    {
        // setEnabledness();
    }
}


void LegalHdrPlugin::setupAndProcess(LegalHdr& p_LegalHdr, const OFX::RenderArguments& p_Args)
{
    // Get the dst image
    std::auto_ptr<OFX::Image> dst(m_DstClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum dstBitDepth = dst->getPixelDepth();
    OFX::PixelComponentEnum dstComponents = dst->getPixelComponents();

    // Get the src image
    std::auto_ptr<OFX::Image> src(m_SrcClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum srcBitDepth = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    // Check to see if the bit depth and number of components are the same
    if ((srcBitDepth != dstBitDepth) || (srcComponents != dstComponents))
    {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
    }

    bool displayOverlay = m_DisplayOverlay->getValue();
    int DisplayOverlay = displayOverlay ? 1 : 0;

    bool lumaOverlay = m_LumaOverlay->getValue();
    int LumaOverlay = lumaOverlay ? 1 : 0;

    double thresholdNits, overlayR, overlayG, overlayB;
    thresholdNits = m_ThresholdNits->getValueAtTime(p_Args.time);
    overlayR = m_OverlayRgbColorR->getValueAtTime(p_Args.time);
    overlayG = m_OverlayRgbColorG->getValueAtTime(p_Args.time);
    overlayB = m_OverlayRgbColorB->getValueAtTime(p_Args.time);

    // Set the scales
    p_LegalHdr.setScales(thresholdNits, overlayR, overlayG, overlayB, DisplayOverlay, LumaOverlay);

    // Set the images
    p_LegalHdr.setDstImg(dst.get());
    p_LegalHdr.setSrcImg(src.get());

    // Setup OpenCL and CUDA Render arguments
    p_LegalHdr.setGPURenderArgs(p_Args);

    // Set the render window
    p_LegalHdr.setRenderWindow(p_Args.renderWindow);

    // Call the base class process member, this will call the derived templated process code
    p_LegalHdr.process();
}

////////////////////////////////////////////////////////////////////////////////

using namespace OFX;

LegalHdrPluginFactory::LegalHdrPluginFactory()
    : OFX::PluginFactoryHelper<LegalHdrPluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
{
}

void LegalHdrPluginFactory::describe(OFX::ImageEffectDescriptor& p_Desc)
{
    // Basic labels
    p_Desc.setLabels(kPluginName, kPluginName, kPluginName);
    p_Desc.setPluginGrouping(kPluginGrouping);
    p_Desc.setPluginDescription(kPluginDescription);

    // Add the supported contexts, only filter at the moment
    p_Desc.addSupportedContext(eContextFilter);
    p_Desc.addSupportedContext(eContextGeneral);

    // Add supported pixel depths
    p_Desc.addSupportedBitDepth(eBitDepthFloat);

    // Set a few flags
    p_Desc.setSingleInstance(false);
    p_Desc.setHostFrameThreading(false);
    p_Desc.setSupportsMultiResolution(kSupportsMultiResolution);
    p_Desc.setSupportsTiles(kSupportsTiles);
    p_Desc.setTemporalClipAccess(false);
    p_Desc.setRenderTwiceAlways(false);
    p_Desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);

    // Setup OpenCL and CUDA render capability flags
//    p_Desc.setSupportsOpenCLRender(true);
    p_Desc.setSupportsCudaRender(true);
//#ifdef __APPLE__
//    p_Desc.setSupportsMetalRender(true);
//#endif
}

static DoubleParamDescriptor* defineScaleParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name, const std::string& p_Label,
                                               const std::string& p_Hint, GroupParamDescriptor* p_Parent)
{
    DoubleParamDescriptor* param = p_Desc.defineDoubleParam(p_Name);
    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(100);
    param->setRange(0, 100);
    param->setIncrement(0.1);
    param->setDisplayRange(0, 100);
    param->setDoubleType(eDoubleTypeScale);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}


static DoubleParamDescriptor* defineNitsParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name, const std::string& p_Label,
                                               const std::string& p_Hint, GroupParamDescriptor* p_Parent)
{
    DoubleParamDescriptor* nits = p_Desc.defineDoubleParam(p_Name);
    nits->setLabels(p_Label, p_Label, p_Label);
    nits->setScriptName(p_Name);
    nits->setHint(p_Hint);
    nits->setDefault(1000);
    nits->setRange(0, 10000);
    nits->setIncrement(10.);
    nits->setDisplayRange(0, 10000);
    nits->setDoubleType(eDoubleTypeScale);
    
    if (p_Parent)
    {
        nits->setParent(*p_Parent);
    }
    
    return nits;
}


void LegalHdrPluginFactory::describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum /*p_Context*/)
{
    // Source clip only in the filter context
    // Create the mandated source clip
    ClipDescriptor* srcClip = p_Desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // Create the mandated output clip
    ClipDescriptor* dstClip = p_Desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentAlpha);
    dstClip->setSupportsTiles(kSupportsTiles);

    // Make some pages and to things in
    PageParamDescriptor* page = p_Desc.definePageParam("Controls");
    
    // Make overall Nits params
    {
        DoubleParamDescriptor* nits = defineNitsParam(p_Desc, "thresholdNits", "Nits Warning Threshold", "Sets warning threshold in Nits relative to PQ image data", 0);
        page->addChild(*nits);
    }
    
        // Add a boolean to enable the overlay
    {
        BooleanParamDescriptor* boolParam = p_Desc.defineBooleanParam("displayOverlay");
        boolParam->setDefault(false);
        boolParam->setHint("Enables warning overlay");
        boolParam->setLabels("Overlay On", "Overlay On", "Overlay On");
        page->addChild(*boolParam);
    }

        // Add a boolean to toggle luma-based warning
    {
        BooleanParamDescriptor* boolParam = p_Desc.defineBooleanParam("lumaOverlay");
        boolParam->setDefault(false);
        boolParam->setHint("Enables luma-based overlay");
        boolParam->setLabels("Luma Overlay", "Luma Overlay", "Luma Overlay");
        page->addChild(*boolParam);
    }

    // Group param to group the overlay channel values
    {
        GroupParamDescriptor* overlayScalesGroup = p_Desc.defineGroupParam("overlayScales");
        overlayScalesGroup->setHint("RGB channel levels for warning overlay");
        overlayScalesGroup->setLabels("Overlay Color", "Overlay Color", "Overlay Color");
    
        // Make the 3 overlay channel value params
        {
            
            DoubleParamDescriptor* param = defineScaleParam(p_Desc, "overlayR", "Red", "Adjusts the red portion of the warning overlay", overlayScalesGroup);
            page->addChild(*param);

            param = defineScaleParam(p_Desc, "overlayG", "Green", "Adjusts the green portion of the warning overlay", overlayScalesGroup);
            page->addChild(*param);

            param = defineScaleParam(p_Desc, "overlayB", "Blue", "Adjusts the blue portion of the warning overlay", overlayScalesGroup);
            page->addChild(*param);

            // param = defineScaleParam(p_Desc, "scaleA", "alpha", "Scales the alpha component of the image", overlayScalesGroup);
            // page->addChild(*param);
        }
    }
}

ImageEffect* LegalHdrPluginFactory::createInstance(OfxImageEffectHandle p_Handle, ContextEnum /*p_Context*/)
{
    return new LegalHdrPlugin(p_Handle);
}

void OFX::Plugin::getPluginIDs(PluginFactoryArray& p_FactoryArray)
{
    static LegalHdrPluginFactory LegalHdrPlugin;
    p_FactoryArray.push_back(&LegalHdrPlugin);
}
