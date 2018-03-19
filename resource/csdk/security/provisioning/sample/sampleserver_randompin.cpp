/******************************************************************
*
* Copyright 2015 Samsung Electronics All Rights Reserved.
*
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************/
///////////////////////////////////////////////////////////////////////
//NOTE :  This sample server is generated based on ocserverbasicops.cpp
///////////////////////////////////////////////////////////////////////
#include "iotivity_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include <signal.h>
#include "ocstack.h"
#include "ocpayload.h"
#include "pinoxmcommon.h"
#if defined(__WITH_TLS__) && defined(WITH_CLOUD)
#include "cloud/cloudresource.h"
#endif // __WITH_TLS__ && WITH_CLOUD

#ifdef HAVE_WINDOWS_H
#include <windows.h>
/** @todo stop-gap for naming issue. Windows.h does not like us to use ERROR */
#ifdef ERROR
#undef ERROR
#endif //ERROR
#endif //HAVE_WINDOWS_H
#include "platform_features.h"
#include "experimental/logger.h"

#define TAG "SAMPLE_RANDOMPIN"

int gQuitFlag = 0;
const char * specVersion = "ocf.1.1.0";

/* Structure to represent a LED resource */
typedef struct LEDRESOURCE{
    OCResourceHandle handle;
    bool state;
    int64_t power;
} LEDResource;

static LEDResource LED;
// This variable determines instance number of the LED resource.
// Used by POST method to create a new instance of LED resource.
static int gCurrLedInstance = 0;
#define SAMPLE_MAX_NUM_POST_INSTANCE  2
static LEDResource gLedInstance[SAMPLE_MAX_NUM_POST_INSTANCE];

char *gResourceUri= (char *)"/a/led";

//Secure Virtual Resource database for Iotivity Server
//It contains Server's Identity and the PSK credentials
//of other devices which the server trusts
static char CRED_FILE[] = "oic_svr_db_server_randompin.dat";

/* Function that creates a new LED resource by calling the
 * OCCreateResource() method.
 */
int createLEDResource (char *uri, LEDResource *ledResource, bool resourceState, int64_t resourcePower);

/* This method converts the payload to JSON format */
OCRepPayload* constructResponse (OCEntityHandlerRequest *ehRequest);

/* Following methods process the PUT, GET, POST
 * requests
 */
OCEntityHandlerResult ProcessGetRequest (OCEntityHandlerRequest *ehRequest,
                                         OCRepPayload **payload);
OCEntityHandlerResult ProcessPutRequest (OCEntityHandlerRequest *ehRequest,
                                         OCRepPayload **payload);
OCEntityHandlerResult ProcessPostRequest (OCEntityHandlerRequest *ehRequest,
                                        OCEntityHandlerResponse *response,
                                        OCRepPayload **payload);

/* Entity Handler callback functions */
OCEntityHandlerResult
OCEntityHandlerCb (OCEntityHandlerFlag flag,
        OCEntityHandlerRequest *entityHandlerRequest,
        void* callbackParam);

const char *getResult(OCStackResult result) {
    switch (result) {
    case OC_STACK_OK:
        return "OC_STACK_OK";
    case OC_STACK_RESOURCE_CREATED:
        return "OC_STACK_RESOURCE_CREATED";
    case OC_STACK_RESOURCE_DELETED:
        return "OC_STACK_RESOURCE_DELETED";
    case OC_STACK_INVALID_URI:
        return "OC_STACK_INVALID_URI";
    case OC_STACK_INVALID_QUERY:
        return "OC_STACK_INVALID_QUERY";
    case OC_STACK_INVALID_IP:
        return "OC_STACK_INVALID_IP";
    case OC_STACK_INVALID_PORT:
        return "OC_STACK_INVALID_PORT";
    case OC_STACK_INVALID_CALLBACK:
        return "OC_STACK_INVALID_CALLBACK";
    case OC_STACK_INVALID_METHOD:
        return "OC_STACK_INVALID_METHOD";
    case OC_STACK_NO_MEMORY:
        return "OC_STACK_NO_MEMORY";
    case OC_STACK_COMM_ERROR:
        return "OC_STACK_COMM_ERROR";
    case OC_STACK_INVALID_PARAM:
        return "OC_STACK_INVALID_PARAM";
    case OC_STACK_NOTIMPL:
        return "OC_STACK_NOTIMPL";
    case OC_STACK_NO_RESOURCE:
        return "OC_STACK_NO_RESOURCE";
    case OC_STACK_RESOURCE_ERROR:
        return "OC_STACK_RESOURCE_ERROR";
    case OC_STACK_SLOW_RESOURCE:
        return "OC_STACK_SLOW_RESOURCE";
    case OC_STACK_NO_OBSERVERS:
        return "OC_STACK_NO_OBSERVERS";
    #ifdef WITH_PRESENCE
    case OC_STACK_PRESENCE_STOPPED:
        return "OC_STACK_PRESENCE_STOPPED";
    #endif
    case OC_STACK_ERROR:
        return "OC_STACK_ERROR";
    default:
        return "UNKNOWN";
    }
}

#ifdef MULTIPLE_OWNER

#include <assert.h>
#include <thread>
#include <chrono>

static bool volatile g_LoopFlag;
static std::thread* oc_process_thread = nullptr;

static void oc_process_loop()
{
    while(g_LoopFlag)
    {
        if (OCProcess() != OC_STACK_OK)
        {
            OIC_LOG(ERROR, TAG, "OCStack process error");
            g_LoopFlag = false;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

static void StartOCProcessThread()
{
    g_LoopFlag = true;
    oc_process_thread = new std::thread(oc_process_loop);
    if(nullptr == oc_process_thread)
    {
        OIC_LOG_V(ERROR, TAG, "%s start thread mail loop returns null, exit.", __func__);
        ::exit(1);
    }
}

static void StopOCProcessThread()
{
    assert(oc_process_thread->joinable() == true);
    g_LoopFlag = false;
    oc_process_thread->join();
}
#endif //MULTIPLE_OWNER

OCRepPayload* getPayload(const char* uri, int64_t power, bool state)
{
    OCRepPayload* payload = OCRepPayloadCreate();
    if(!payload)
    {
        OIC_LOG(ERROR, TAG, "Failed to allocate Payload");
        return NULL;
    }

    OCRepPayloadSetUri(payload, uri);
    OCRepPayloadSetPropBool(payload, "state", state);
    OCRepPayloadSetPropInt(payload, "power", power);

    return payload;
}

//This function takes the request as an input and returns the response
OCRepPayload* constructResponse (OCEntityHandlerRequest *ehRequest)
{
    if(ehRequest->payload && ehRequest->payload->type != PAYLOAD_TYPE_REPRESENTATION)
    {
        OIC_LOG(ERROR, TAG, "Incoming payload not a representation");
        return NULL;
    }

    OCRepPayload* input = (OCRepPayload*)(ehRequest->payload);

    LEDResource *currLEDResource = &LED;

    if (ehRequest->resource == gLedInstance[0].handle)
    {
        currLEDResource = &gLedInstance[0];
        gResourceUri = (char *) "/a/led/0";
    }
    else if (ehRequest->resource == gLedInstance[1].handle)
    {
        currLEDResource = &gLedInstance[1];
        gResourceUri = (char *) "/a/led/1";
    }

    if(OC_REST_PUT == ehRequest->method)
    {
        // Get pointer to query
        int64_t pow;
        if(OCRepPayloadGetPropInt(input, "power", &pow))
        {
            currLEDResource->power = pow;
        }

        bool state;
        if(OCRepPayloadGetPropBool(input, "state", &state))
        {
            currLEDResource->state = state;
        }
    }

    return getPayload(gResourceUri, currLEDResource->power, currLEDResource->state);
}

OCEntityHandlerResult ProcessGetRequest (OCEntityHandlerRequest *ehRequest,
        OCRepPayload **payload)
{
    OCEntityHandlerResult ehResult;

    OCRepPayload *getResp = constructResponse(ehRequest);

    if(getResp)
    {
        *payload = getResp;
        ehResult = OC_EH_OK;
    }
    else
    {
        ehResult = OC_EH_ERROR;
    }

    return ehResult;
}

OCEntityHandlerResult ProcessPutRequest (OCEntityHandlerRequest *ehRequest,
        OCRepPayload **payload)
{
    OCEntityHandlerResult ehResult;

    OCRepPayload *putResp = constructResponse(ehRequest);

    if(putResp)
    {
        *payload = putResp;
        ehResult = OC_EH_OK;
    }
    else
    {
        ehResult = OC_EH_ERROR;
    }

    return ehResult;
}

OCEntityHandlerResult ProcessPostRequest (OCEntityHandlerRequest *ehRequest,
        OCEntityHandlerResponse *response, OCRepPayload **payload)
{
    OCRepPayload *respPLPost_led = NULL;
    OCEntityHandlerResult ehResult = OC_EH_OK;

    /*
     * The entity handler determines how to process a POST request.
     * Per the REST paradigm, POST can also be used to update representation of existing
     * resource or create a new resource.
     * In the sample below, if the POST is for /a/led then a new instance of the LED
     * resource is created with default representation (if representation is included in
     * POST payload it can be used as initial values) as long as the instance is
     * lesser than max new instance count. Once max instance count is reached, POST on
     * /a/led updated the representation of /a/led (just like PUT)
     */

    if (ehRequest->resource == LED.handle)
    {
        if (gCurrLedInstance < SAMPLE_MAX_NUM_POST_INSTANCE)
        {
            // Create new LED instance
            char newLedUri[15] = "/a/led/";
            size_t newLedUriLength = strlen(newLedUri);
            snprintf (newLedUri + newLedUriLength, sizeof(newLedUri)-newLedUriLength, "%d", gCurrLedInstance);

            respPLPost_led = OCRepPayloadCreate();
            OCRepPayloadSetUri(respPLPost_led, gResourceUri);
            OCRepPayloadSetPropString(respPLPost_led, "createduri", newLedUri);

            if (0 == createLEDResource (newLedUri, &gLedInstance[gCurrLedInstance], false, 0))
            {
                OIC_LOG (INFO, TAG, "Created new LED instance");
                gLedInstance[gCurrLedInstance].state = 0;
                gLedInstance[gCurrLedInstance].power = 0;
                gCurrLedInstance++;
                strncpy ((char *)response->resourceUri, newLedUri, sizeof(response->resourceUri));
                ehResult = OC_EH_RESOURCE_CREATED;
            }
        }
        else
        {
            respPLPost_led = constructResponse(ehRequest);
        }
    }
    else
    {
        for (int i = 0; i < SAMPLE_MAX_NUM_POST_INSTANCE; i++)
        {
            if (ehRequest->resource == gLedInstance[i].handle)
            {
                if (i == 0)
                {
                    respPLPost_led = constructResponse(ehRequest);
                    break;
                }
                else if (i == 1)
                {
                    respPLPost_led = constructResponse(ehRequest);
                }
            }
        }
    }

    if (respPLPost_led != NULL)
    {
        *payload = respPLPost_led;
        ehResult = OC_EH_OK;
    }
    else
    {
        OIC_LOG_V (INFO, TAG, "Payload was NULL");
        ehResult = OC_EH_ERROR;
    }

    return ehResult;
}

OCEntityHandlerResult
OCEntityHandlerCb (OCEntityHandlerFlag flag,
        OCEntityHandlerRequest *entityHandlerRequest,
        void* callbackParam)
{
    OIC_LOG_V (INFO, TAG, "Inside entity handler - flags: 0x%x", flag);
    (void)callbackParam;
    OCEntityHandlerResult ehResult = OC_EH_ERROR;

    OCEntityHandlerResponse response;
    memset(&response, 0, sizeof(response));

    // Validate pointer
    if (!entityHandlerRequest)
    {
        OIC_LOG (ERROR, TAG, "Invalid request pointer");
        return OC_EH_ERROR;
    }

    OCRepPayload* payload = NULL;

    if (flag & OC_REQUEST_FLAG)
    {
        OIC_LOG (INFO, TAG, "Flag includes OC_REQUEST_FLAG");
        if (entityHandlerRequest)
        {
            if (OC_REST_GET == entityHandlerRequest->method)
            {
                OIC_LOG (INFO, TAG, "Received OC_REST_GET from client");
                ehResult = ProcessGetRequest (entityHandlerRequest, &payload);
            }
            else if (OC_REST_PUT == entityHandlerRequest->method)
            {
                OIC_LOG (INFO, TAG, "Received OC_REST_PUT from client");
                ehResult = ProcessPutRequest (entityHandlerRequest, &payload);
            }
            else if (OC_REST_POST == entityHandlerRequest->method)
            {
                OIC_LOG (INFO, TAG, "Received OC_REST_POST from client");
                ehResult = ProcessPostRequest (entityHandlerRequest, &response, &payload);
            }
            else
            {
                OIC_LOG_V (INFO, TAG, "Received unsupported method %d from client",
                        entityHandlerRequest->method);
                ehResult = OC_EH_ERROR;
            }

            if (ehResult == OC_EH_OK && ehResult != OC_EH_FORBIDDEN)
            {
                // Format the response.  Note this requires some info about the request
                response.requestHandle = entityHandlerRequest->requestHandle;
                response.ehResult = ehResult;
                response.payload = (OCPayload*)(payload);
                response.numSendVendorSpecificHeaderOptions = 0;
                memset(response.sendVendorSpecificHeaderOptions, 0,
                       sizeof(response.sendVendorSpecificHeaderOptions));
                memset(response.resourceUri, 0, sizeof(response.resourceUri));
                // Indicate that response is NOT in a persistent buffer
                response.persistentBufferFlag = 0;

                // Send the response
                if (OCDoResponse(&response) != OC_STACK_OK)
                {
                    OIC_LOG(ERROR, TAG, "Error sending response");
                    ehResult = OC_EH_ERROR;
                }
            }
        }
    }

    OCPayloadDestroy(response.payload);
    return ehResult;
}

/* SIGINT handler: set gQuitFlag to 1 for graceful termination */
void handleSigInt(int signum)
{
    if (signum == SIGINT)
    {
        gQuitFlag = 1;
    }
}

FILE* server_fopen(const char *path, const char *mode)
{
    if (0 == strcmp(path, OC_SECURITY_DB_DAT_FILE_NAME))
    {
        return fopen(CRED_FILE, mode);
    }
    else
    {
        // unknown file request - try to open it by the given name
        return fopen(path, mode);
    }
}

void OC_CALL DisplayPinCB(char *pin, size_t pinSize, void *context)
{
    OC_UNUSED(context);

    if(NULL == pin || pinSize <= 0)
    {
        OIC_LOG(INFO, TAG, "Invalid PIN");
        return;
    }

    OIC_LOG(INFO, TAG, "============================");
    OIC_LOG_V(INFO, TAG, "    PIN CODE : %s", pin);
    OIC_LOG(INFO, TAG, "============================");
}

void OC_CALL ClosePinDisplayCB(void)
{
    OIC_LOG(INFO, TAG, "============================");
    OIC_LOG(INFO, TAG, "    PIN DISPLAY CLOSED.");
    OIC_LOG(INFO, TAG, "============================");
}

int main()
{
    OIC_LOG(DEBUG, TAG, "OCServer is starting...");

    // Initialize Persistent Storage for SVR database
    OCPersistentStorage ps = {server_fopen, fread, fwrite, fclose, remove};
    OCRegisterPersistentStorageHandler(&ps);

    if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack init error");
        return 0;
    }
#if defined(__WITH_TLS__) && defined(WITH_CLOUD)
    if (OC_STACK_OK != InitCloudResource())
    {
        OIC_LOG(ERROR, TAG, "Cloud init error");
    }
#endif // __WITH_TLS__ && WITH_CLOUD

   /**
     * If the server supports random pin based ownership transfer, the callback
     * to display a PIN should be registered before running the server.
     */
    SetDisplayPinWithContextCB(DisplayPinCB, NULL);

    /**
     * If the server supports random pin based OTM,
     * the callback to close PIN display can be registered.
     * This callback will be invoked when random PIN based OTM is done.
     */
    SetClosePinDisplayCB(ClosePinDisplayCB);

    /**
     * Random PIN generation policy can be changed through SetRandomPinPolicy() API.
     * first param : byte length of random PIN ( 4 <= first param <= 32)
     * second param : PIN type (This is bitmask)
     */
    if(OC_STACK_OK != SetRandomPinPolicy(8, NUM_PIN))
    {
        OIC_LOG(ERROR, TAG, "Failed to setting PIN policy");
        return 0;
    }
    OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_SPEC_VERSION, (void*) specVersion);

    /*
     * Declare and create the example resource: LED
     */
    createLEDResource(gResourceUri, &LED, false, 0);

    // Break from loop with Ctrl-C
    OIC_LOG(INFO, TAG, "Entering ocserver main loop...");
    signal(SIGINT, handleSigInt);

#ifdef MULTIPLE_OWNER
    StartOCProcessThread();

    while(!gQuitFlag)
    {
        printf("Press 'G' to generate random PIN...\n");
        printf("Press 'E' to exit...\n");
        char in = (char)getchar();
        if('G' == in || 'g' == in)
        {
            char ranPin[OXM_RANDOM_PIN_MAX_SIZE + 1] = {0};
            GeneratePin(ranPin, sizeof(ranPin));
        }
        if('E' == in || 'e' == in)
        {
            break;
        }
    }

    StopOCProcessThread();
#else
    struct timespec timeout;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = 100000000L;

    while (!gQuitFlag)
    {
        if (OCProcess() != OC_STACK_OK)
        {
            OIC_LOG(ERROR, TAG, "OCStack process error");
            return 0;
        }
        nanosleep(&timeout, NULL);
    }
#endif //MULTIPLE_OWNER

    OIC_LOG(INFO, TAG, "Exiting ocserver main loop...");
#if defined(__WITH_TLS__) && defined(WITH_CLOUD)
    DeInitCloudResource();
#endif // __WITH_TLS__ && WITH_CLOUD

    if (OCStop() != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack process error");
    }

    return 0;
}

int createLEDResource (char *uri, LEDResource *ledResource, bool resourceState, int64_t resourcePower)
{
    if (!uri)
    {
        OIC_LOG(ERROR, TAG, "Resource URI cannot be NULL");
        return -1;
    }

    ledResource->state = resourceState;
    ledResource->power= resourcePower;
    OCStackResult res = OCCreateResource(&(ledResource->handle),
            "core.led",
            OC_RSRVD_INTERFACE_DEFAULT,
            uri,
            OCEntityHandlerCb,
            NULL,
            OC_DISCOVERABLE|OC_OBSERVABLE | OC_SECURE);
    OIC_LOG_V(INFO, TAG, "Created LED resource with result: %s", getResult(res));

    return 0;
}
