/*******************************************************************************
 *
 * Copyright(C) 2017 Renesas Electronics Corporation.
 * Copyright 2016 Cadence Design Systems, Inc.
 * Copyright (C) 2009 The Android Open Source Project
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/
/*******************************************************************************
 * xa-gst-omx.c
 *
 * Xtensa OMX-IL core for gst-omx
 ******************************************************************************/

#define MODULE_TAG                      XA_PLUGIN

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "xf-ap.h"
#include <OMX_Component.h>

/*******************************************************************************
 * Tracing configuration
 ******************************************************************************/

TRACE_TAG(INIT, 1);

/*******************************************************************************
 * Components factories
 ******************************************************************************/

extern OMX_ERRORTYPE AACDEC_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);
extern OMX_ERRORTYPE MP3DEC_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);
extern OMX_ERRORTYPE AACENC_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);
extern OMX_ERRORTYPE VORBISDEC_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);
extern OMX_ERRORTYPE RENDERER_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);
extern OMX_ERRORTYPE CAPTURE_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);
extern OMX_ERRORTYPE EQUALIZER_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);

extern OMX_ERRORTYPE TDM_RENDERER_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);
extern OMX_ERRORTYPE TDM_CAPTURE_ComponentCreate(xf_proxy_t *proxy, OMX_HANDLETYPE *hComponent, OMX_PTR appData, const OMX_CALLBACKTYPE *callbacks);

/*******************************************************************************
 * Global variables
 ******************************************************************************/

/* ...proxy handle */
static xf_proxy_t       xf_proxy;

/* ...list of the components */
static const struct {
    const char     *mName;
    const char     *mRole;
    OMX_ERRORTYPE (*factory)(xf_proxy_t *, OMX_HANDLETYPE *, OMX_PTR, const OMX_CALLBACKTYPE *);
} kComponents[] = {
    { "OMX.xa.aac.decoder",     "audio_decoder.aac",    AACDEC_ComponentCreate },
    { "OMX.xa.mp3.decoder",     "audio_decoder.mp3",    MP3DEC_ComponentCreate },
    { "OMX.xa.aac.encoder",     "audio_encoder.aac",    AACENC_ComponentCreate },
    { "OMX.xa.vorbis.decoder",  "audio_decoder.vorbis", VORBISDEC_ComponentCreate },
    { "OMX.RENESAS.AUDIO.DSP.RENDERER",  "audio_renderer.pcm", RENDERER_ComponentCreate },
    { "OMX.RENESAS.AUDIO.DSP.CAPTURE",  "audio_capture.pcm", CAPTURE_ComponentCreate },
    { "OMX.RENESAS.AUDIO.DSP.EQUALIZER",  "audio_processor.pcm.equalizer", EQUALIZER_ComponentCreate },
    { "OMX.RENESAS.AUDIO.DSP.TDMRENDERER",  "audio_tdm_renderer.pcm", TDM_RENDERER_ComponentCreate },
    { "OMX.RENESAS.AUDIO.DSP.TDMCAPTURE",  "audio_tdm_capture.pcm", TDM_CAPTURE_ComponentCreate },
};

/* ...total number of components */
#define kNumComponents      (sizeof(kComponents) / sizeof(kComponents[0]))

/*******************************************************************************
 * Internal functions
 ******************************************************************************/

/* ...size of auxiliary pool - effectively, maximal number of components */
#define XA_AUX_POOL_SIZE            16

/* ...size of auxiliary buffer */
#define XA_AUX_POOL_MSG_LENGTH      256

/* ...proxy instantiation */
static inline int xaomx_init(xf_proxy_t *proxy)
{
    int     r;
    
    /* ...create proxy handle */
    XF_CHK_API(xf_proxy_init(proxy, 0));
    
    /* ...allocate auxiliary buffer pool */
    if ((r = xf_pool_alloc(proxy, XA_AUX_POOL_SIZE, XA_AUX_POOL_MSG_LENGTH, XF_POOL_AUX, &proxy->aux)) != 0) {
        xf_proxy_close(proxy);
        return XF_CHK_API(r);
    }
    
    TRACE(INIT, _b("Proxy initialized"));
    
    return 0;
}

/* ...proxy deinitialization */
static inline void xaomx_deinit(xf_proxy_t *proxy)
{
    /* ...destroy auxiliary pool */
    xf_pool_free(proxy->aux), proxy->aux = NULL;
    
    /* ...close proxy interface */
    xf_proxy_close(proxy);

    TRACE(INIT, _b("Proxy destroyed"));
}

/*******************************************************************************
 * Entry points
 ******************************************************************************/

/* ...OMX core initialization */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    /* ...make sure only single instance of the proxy is created */   
    XF_CHK_ERR(xf_proxy.aux == NULL, OMX_ErrorInsufficientResources);

    /* ...create proxy handle */
    XF_CHK_ERR(xaomx_init(&xf_proxy) == 0, OMX_ErrorUndefined);

    return OMX_ErrorNone;
}

/* ...OMX core deinitialization */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    /* ...make sure the instance of proxy has been created */
    XF_CHK_ERR(xf_proxy.aux != NULL, OMX_ErrorUndefined);
    
    /* ...close proxy interface */
    xaomx_deinit(&xf_proxy);
    
    return OMX_ErrorNone;
}

/* ...component instantiation */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE* pCallBacks)
{
    int             i;
    
    TRACE(INIT, _b("Create component: %s"), cComponentName);

    /* ...check componentname is not null */
    XF_CHK_ERR(cComponentName != NULL, OMX_ErrorInvalidComponentName);

    /* ...check the proxy is initialized */
    XF_CHK_ERR(xf_proxy.aux != NULL, OMX_ErrorInvalidState);
    
    /* ...find component factory */
    for (i = 0; i < kNumComponents; ++i) {
        if (strcmp(cComponentName, kComponents[i].mName))
            continue;

        return kComponents[i].factory(&xf_proxy, pHandle, pAppData, pCallBacks);
    }

    return OMX_ErrorInvalidComponentName;
}

/* ...component destructor */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(OMX_HANDLETYPE hComponent)
{
    /* ...check component is not null */
    XF_CHK_ERR(hComponent != NULL, OMX_ErrorBadParameter);

    return ((OMX_COMPONENTTYPE *)hComponent)->ComponentDeInit(hComponent);
}

/* ...OMX setup tunnel */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(OMX_HANDLETYPE hOutput,
	OMX_U32 nPortOutput, OMX_HANDLETYPE hInput, OMX_U32 nPortInput)
{

	OMX_ERRORTYPE ret;
	OMX_COMPONENTTYPE* component;
	OMX_TUNNELSETUPTYPE tunnelSetup;

	tunnelSetup.nTunnelFlags = 0;
	tunnelSetup.eSupplier = OMX_BufferSupplyUnspecified;

	/* ...parameter checking */
	XF_CHK_ERR(hOutput || hInput, OMX_ErrorBadParameter);

	/* ...set ouput port */
	if(hOutput)
	{
		component = (OMX_COMPONENTTYPE*)hOutput;
		ret = (component->ComponentTunnelRequest)(hOutput, nPortOutput, hInput, nPortInput, &tunnelSetup);
		if (ret != OMX_ErrorNone)
		{
			TRACE(INIT, _b("Tunneling failed: output port rejected - err = %x"), ret);
			return ret;
		}
	}
	TRACE(INIT, _b("First stage of tunneling acheived:"));
	TRACE(INIT, _b("       - supplier proposed = %i"), tunnelSetup.eSupplier);
	TRACE(INIT, _b("       - flags             = %i"), (int)tunnelSetup.nTunnelFlags);

	/* ...set input port */
	if (hInput)
	{
		component = (OMX_COMPONENTTYPE*)hInput;
		ret = (component->ComponentTunnelRequest)(hInput, nPortInput, hOutput, nPortOutput, &tunnelSetup);
		if (ret != OMX_ErrorNone)
		{
			TRACE(INIT, _b("Tunneling failed: input port rejected - err = %08x"), ret);
			/* ...remove output port if necessary */
			if(hOutput)
			{
				component = (OMX_COMPONENTTYPE*)hOutput;
				ret = (component->ComponentTunnelRequest)(hOutput, nPortOutput, NULL, 0, &tunnelSetup);
				if (ret != OMX_ErrorNone)
				{
					TRACE(INIT, _b("Cannot remove output port"), __func__);
					return OMX_ErrorUndefined;
				}
			}
			return ret;
		}
	}
	TRACE(INIT, _b("Second stage of tunneling acheived:\n"));
	TRACE(INIT, _b("       - supplier proposed = %i"), (int)tunnelSetup.eSupplier);
	TRACE(INIT, _b("       - flags             = %i"), (int)tunnelSetup.nTunnelFlags);
	TRACE(INIT, _b("%s finished"), __func__);
	return OMX_ErrorNone;
}

/* ...OMX teardown tunnel */ 
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_TeardownTunnel(OMX_HANDLETYPE hOutput, 
	OMX_U32 nPortOutput, OMX_HANDLETYPE hInput, OMX_U32 nPortInput)
{

	OMX_ERRORTYPE ret;
	OMX_COMPONENTTYPE* component;
	OMX_TUNNELSETUPTYPE tunnelSetup;

	tunnelSetup.nTunnelFlags = 0;
	tunnelSetup.eSupplier = OMX_BufferSupplyUnspecified;

	/* ...parameter checking */
	XF_CHK_ERR(hOutput && hInput, OMX_ErrorBadParameter);

	/* ...teardown input port */
	if(hInput)
	{
		component = (OMX_COMPONENTTYPE*)hInput;
		ret = (component->ComponentTunnelRequest)(hInput, nPortInput, NULL, 0, &tunnelSetup);
		if (ret != OMX_ErrorNone)
		{
			TRACE(INIT, _b("Teardown failed: input port rejected - err = %x"), ret);
			return ret;
		}
	}

	/* ...teardown output port */
	if(hOutput)
	{
		component = (OMX_COMPONENTTYPE*)hOutput;
		ret =(component->ComponentTunnelRequest)(hOutput, nPortOutput, NULL, 0, &tunnelSetup);
		if (ret != OMX_ErrorNone)
		{
			TRACE(INIT, _b("Teardown failed: output port rejected - err = %x"), ret);
			return ret;
		}
	}

	TRACE(INIT, _b("%s finished"), __func__);
	return OMX_ErrorNone;
}

