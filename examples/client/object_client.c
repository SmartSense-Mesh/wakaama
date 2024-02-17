#include <string.h>
#include <stdio.h>
#include "liblwm2m.h"

typedef struct _client_instance_ {
    struct _client_instance_ * next;
    uint16_t                   instanceId;
    char *                     uri;
} client_instance_t;

static uint8_t prv_get_value(lwm2m_data_t *dataP, client_instance_t *targetP) {
    switch (dataP->id) {
    case LWM2M_CLIENT_URI_ID:
        lwm2m_data_encode_string(targetP->uri, dataP);
        return COAP_205_CONTENT;
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_client_read(lwm2m_context_t *contextP,
                                 uint16_t instanceId,
                                 int * numDataP,
                                 lwm2m_data_t ** dataArrayP,
                                 lwm2m_object_t * objectP) {
    client_instance_t * targetP;
    uint8_t result;
    int i;

    (void)contextP;

    targetP = (client_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (*numDataP == 0) {
        uint16_t resList[] = {
            LWM2M_CLIENT_URI_ID
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

static uint8_t prv_client_write(lwm2m_context_t *contextP,
                                uint16_t instanceId,
                                int numData,
                                lwm2m_data_t * dataArray,
                                lwm2m_object_t * objectP,
                                lwm2m_write_type_t writeType) {
    client_instance_t * targetP;
    int i;
    uint8_t result = COAP_204_CHANGED;

    /* unused parameter */
    (void)contextP;

    /* All write types are ignored. They don't apply during bootstrap. */
    (void)writeType;

    targetP = (client_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
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
        case LWM2M_CLIENT_URI_ID:
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

static uint8_t prv_client_delete(lwm2m_context_t *contextP,
                                   uint16_t id,
                                   lwm2m_object_t * objectP) {
    client_instance_t * targetP;

    /* unused parameter */
    (void)contextP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    if (NULL != targetP->uri)
    {
        lwm2m_free(targetP->uri);
    }

    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_client_create(lwm2m_context_t *contextP,
                                   uint16_t instanceId,
                                   int numData,
                                   lwm2m_data_t * dataArray,
                                   lwm2m_object_t * objectP)
{
    client_instance_t * targetP;
    uint8_t result;

    targetP = (client_instance_t *)lwm2m_malloc(sizeof(client_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(client_instance_t));

    targetP->instanceId = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_client_write(contextP, instanceId, numData, dataArray, objectP, LWM2M_WRITE_REPLACE_RESOURCES);

    if (result != COAP_204_CHANGED)
    {
        (void)prv_client_delete(contextP, instanceId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

void copy_client_object(lwm2m_object_t * objectDest, lwm2m_object_t * objectSrc) {
    memcpy(objectDest, objectSrc, sizeof(lwm2m_object_t));
    objectDest->instanceList = NULL;
    objectDest->userData = NULL;
    client_instance_t * instanceSrc = (client_instance_t *)objectSrc->instanceList;
    client_instance_t * previousInstanceDest = NULL;
    while (instanceSrc != NULL)
    {
        client_instance_t * instanceDest = (client_instance_t *)lwm2m_malloc(sizeof(client_instance_t));
        if (NULL == instanceDest)
        {
            return;
        }
        memcpy(instanceDest, instanceSrc, sizeof(client_instance_t));
        instanceDest->uri = (char*)lwm2m_malloc(strlen(instanceSrc->uri) + 1);
        strcpy(instanceDest->uri, instanceSrc->uri);
        instanceSrc = (client_instance_t *)instanceSrc->next;
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

void display_client_object(lwm2m_object_t * object)
{
    fprintf(stdout, "  /%u: Client object, instances:\r\n", object->objID);
    client_instance_t * instance = (client_instance_t *)object->instanceList;
    while (instance != NULL)
    {
        fprintf(stdout, "    /%u/%u: instanceId: %u, uri: %s\r\n",
                object->objID, instance->instanceId,
                instance->instanceId, instance->uri);
        instance = (client_instance_t *)instance->next;
    }
}

void clean_client_object(lwm2m_object_t * objectP)
{
    while (objectP->instanceList != NULL)
    {
        client_instance_t * instance = (client_instance_t *)objectP->instanceList;
        objectP->instanceList = objectP->instanceList->next;
        if (NULL != instance->uri)
        {
            lwm2m_free(instance->uri);
        }
        lwm2m_free(instance);
    }
}

lwm2m_object_t * get_client_object(void) {
    lwm2m_object_t * clientObj;

    clientObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != clientObj) {
        memset(clientObj, 0, sizeof(lwm2m_object_t));

        clientObj->objID = LWM2M_CLIENT_OBJECT_ID;
        clientObj->readFunc = prv_client_read;
        clientObj->writeFunc = prv_client_write;
        clientObj->createFunc = prv_client_create;
        clientObj->deleteFunc = prv_client_delete;
    }

    return clientObj;
}

void create_client_instance(lwm2m_object_t * clientObj, const char* serverUri) {
    client_instance_t * targetP = (client_instance_t *)lwm2m_malloc(sizeof(client_instance_t));
    static int instanceId = 0;
    if (NULL == targetP)
    {
        printf("Couldn't create the instance\n");
        lwm2m_free(clientObj);
        return;
    }

    memset(targetP, 0, sizeof(client_instance_t));
    targetP->instanceId = instanceId;
    targetP->uri = (char*)lwm2m_malloc(strlen(serverUri)+1);
    strcpy(targetP->uri, serverUri);

    clientObj->instanceList = LWM2M_LIST_ADD(clientObj->instanceList, targetP);
    instanceId += 1;
}

char * get_client_uri(lwm2m_object_t * objectP,
                     uint16_t secObjInstID)
{
    client_instance_t * targetP = (client_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);

    if (NULL != targetP)
    {
        return lwm2m_strdup(targetP->uri);
    }

    return NULL;
}

// Return the node with URI 'uri' from the list 'head' or NULL if not found
lwm2m_list_t* find_by_uri(lwm2m_list_t * head, const char* uri) {
    while (NULL != head && strncmp(((client_instance_t *) head)->uri, uri, strlen(uri)) != 0)
    {
        head = head->next;
    }

    if (NULL != head && strncmp(((client_instance_t *) head)->uri, uri, strlen(uri)) == 0) return head;

    return NULL;
}
