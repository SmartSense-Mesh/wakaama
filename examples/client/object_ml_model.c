#include <string.h>
#include <stdio.h>
#include "liblwm2m.h"

typedef struct _ml_model_instance_ {
    struct _ml_model_instance_ * next;
    uint16_t                   instanceId;
    char *                     uri;
    char *                     model;
} ml_model_instance_t;

static uint8_t prv_get_value(lwm2m_data_t *dataP, ml_model_instance_t *targetP) {
    switch (dataP->id) {
    case LWM2M_ML_MODEL_URI_ID:
        lwm2m_data_encode_string(targetP->uri, dataP);
        return COAP_205_CONTENT;
    case LWM2M_ML_MODEL_MODEL_ID:
        lwm2m_data_encode_string(targetP->model, dataP);
        return COAP_205_CONTENT;
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_ml_model_read(lwm2m_context_t *contextP,
                               uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArrayP,
                               lwm2m_object_t * objectP) {
    ml_model_instance_t * targetP;
    uint8_t result;
    int i;
    
    (void)contextP;

    targetP = (ml_model_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (*numDataP == 0) {
        uint16_t resList[] = {
            LWM2M_ML_MODEL_URI_ID,
            LWM2M_ML_MODEL_MODEL_ID
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }
    i = 0;
    do
    {
        if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            result = COAP_404_NOT_FOUND;
        }
        else
        {
            result = prv_get_value((*dataArrayP) + i, targetP);
        }
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_ml_model_write(lwm2m_context_t *contextP,
                                uint16_t instanceId,
                                int numData,
                                lwm2m_data_t * dataArray,
                                lwm2m_object_t * objectP,
                                lwm2m_write_type_t writeType) {
    ml_model_instance_t * targetP;
    int i;
    uint8_t result = COAP_204_CHANGED;

    /* unused parameter */
    (void)contextP;

    /* All write types are ignored. They don't apply during bootstrap. */
    (void)writeType;

    targetP = (ml_model_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
    {
        return COAP_404_NOT_FOUND;
    }

    i = 0;
    do {
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            result = COAP_404_NOT_FOUND;
            continue;
        }

        switch (dataArray[i].id) {
        case LWM2M_ML_MODEL_URI_ID:
            if (targetP->uri != NULL) lwm2m_free(targetP->uri);
            targetP->uri = (char *)lwm2m_malloc(dataArray[i].value.asBuffer.length + 1);
            memset(targetP->uri, 0, dataArray[i].value.asBuffer.length + 1);
            if (targetP->uri != NULL)
            {
                strncpy(targetP->uri, (char*)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_500_INTERNAL_SERVER_ERROR;
            }
            break;
        case LWM2M_ML_MODEL_MODEL_ID:
            if (targetP->uri != NULL) lwm2m_free(targetP->uri);
            targetP->uri = (char *)lwm2m_malloc(dataArray[i].value.asBuffer.length + 1);
            memset(targetP->uri, 0, dataArray[i].value.asBuffer.length + 1);
            if (targetP->uri != NULL)
            {
                strncpy(targetP->uri, (char*)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_500_INTERNAL_SERVER_ERROR;
            }
            break;
        default:
            return COAP_400_BAD_REQUEST;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_ml_model_delete(lwm2m_context_t *contextP,
                                 uint16_t id,
                                 lwm2m_object_t * objectP) {
    ml_model_instance_t * targetP;

    /* unused parameter */
    (void)contextP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    if (NULL != targetP->uri)
    {
        lwm2m_free(targetP->uri);
    }
    if (NULL != targetP->model)
    {
        lwm2m_free(targetP->model);
    }

    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_ml_model_create(lwm2m_context_t *contextP,
                                 uint16_t instanceId,
                                 int numData,
                                 lwm2m_data_t * dataArray,
                                 lwm2m_object_t * objectP)
{
    ml_model_instance_t * targetP;
    uint8_t result;

    targetP = (ml_model_instance_t *)lwm2m_malloc(sizeof(ml_model_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(ml_model_instance_t));

    targetP->instanceId = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_ml_model_write(contextP, instanceId, numData, dataArray, objectP, LWM2M_WRITE_REPLACE_RESOURCES);

    if (result != COAP_204_CHANGED)
    {
        (void)prv_ml_model_delete(contextP, instanceId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

void copy_ml_model_object(lwm2m_object_t * objectDest, lwm2m_object_t * objectSrc) {
    memcpy(objectDest, objectSrc, sizeof(lwm2m_object_t));
    objectDest->instanceList = NULL;
    objectDest->userData = NULL;
    ml_model_instance_t * instanceSrc = (ml_model_instance_t *)objectSrc->instanceList;
    ml_model_instance_t * previousInstanceDest = NULL;
    while (instanceSrc != NULL)
    {
        ml_model_instance_t * instanceDest = (ml_model_instance_t *)lwm2m_malloc(sizeof(ml_model_instance_t));
        if (NULL == instanceDest)
        {
            return;
        }
        memcpy(instanceDest, instanceSrc, sizeof(ml_model_instance_t));
        instanceDest->uri = (char*)lwm2m_malloc(strlen(instanceSrc->uri) + 1);
        strcpy(instanceDest->uri, instanceSrc->uri);
        instanceDest->model = (char*)lwm2m_malloc(strlen(instanceSrc->model) + 1);
        strcpy(instanceDest->model, instanceSrc->model);
        instanceSrc = (ml_model_instance_t *)instanceSrc->next;
        if (previousInstanceDest == NULL)
        {
            objectDest->instanceList = (lwm2m_list_t *)instanceDest;
        }
        else
        {
            previousInstanceDest->next = instanceDest;
        }
        previousInstanceDest = instanceDest;
    }
}

void display_ml_model_object(lwm2m_object_t * object)
{
    fprintf(stdout, "  /%u: ml_model object, instances:\r\n", object->objID);
    ml_model_instance_t * instance = (ml_model_instance_t *)object->instanceList;
    while (instance != NULL)
    {
        fprintf(stdout, "    /%u/%u: instanceId: %u, uri: %s, model: %s\r\n",
                object->objID, instance->instanceId,
                instance->instanceId, instance->uri, instance->model);
        instance = (ml_model_instance_t *)instance->next;
    }
}

void clean_ml_model_object(lwm2m_object_t * objectP)
{
    while (objectP->instanceList != NULL)
    {
        ml_model_instance_t * instance = (ml_model_instance_t *)objectP->instanceList;
        objectP->instanceList = objectP->instanceList->next;
        if (NULL != instance->uri)
        {
            lwm2m_free(instance->uri);
        }
        if (NULL != instance->model)
        {
            lwm2m_free(instance->model);
        }
        lwm2m_free(instance);
    }
}

lwm2m_object_t * get_ml_model_object(void) {
    lwm2m_object_t * ml_modelObj;

    ml_modelObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != ml_modelObj) {
        memset(ml_modelObj, 0, sizeof(lwm2m_object_t));

        ml_modelObj->objID = LWM2M_ML_MODEL_OBJECT_ID;
        ml_modelObj->readFunc = prv_ml_model_read;
        ml_modelObj->writeFunc = prv_ml_model_write;
        ml_modelObj->createFunc = prv_ml_model_create;
        ml_modelObj->deleteFunc = prv_ml_model_delete;
    }

    return ml_modelObj;
}

void create_ml_model_instance(lwm2m_object_t * ml_modelObj, const char* serverUri, const char* model) {
    ml_model_instance_t * targetP = (ml_model_instance_t *)lwm2m_malloc(sizeof(ml_model_instance_t));
    static int instanceId = 0;
    if (NULL == targetP)
    {
        printf("Couldn't create ML model the instance\n");
        lwm2m_free(ml_modelObj);
        return;
    }

    memset(targetP, 0, sizeof(ml_model_instance_t));
    targetP->instanceId = instanceId;

    targetP->uri = (char*)lwm2m_malloc(strlen(serverUri)+1);
    strcpy(targetP->uri, serverUri);

    targetP->model = (char*)lwm2m_malloc(strlen(model)+1);
    strcpy(targetP->model, model);

    ml_modelObj->instanceList = LWM2M_LIST_ADD(ml_modelObj->instanceList, targetP);
    instanceId += 1;
}

char * get_ml_model_uri(lwm2m_object_t * objectP,
                     uint16_t secObjInstID)
{
    ml_model_instance_t * targetP = (ml_model_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);

    if (NULL != targetP)
    {
        return lwm2m_strdup(targetP->uri);
    }

    return NULL;
}

