/*--------------------------------------------------------------------------*/
/*  Includes                                                                */
/*--------------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"

#include "uITRON.h"

#define __GLOBAL_DEFINITION_USER_DEFINITIONS__
#include "UserDefinitions.h"

/*--------------------------------------------------------------------------*/
/*  Macro and Typedef Definitions                                           */
/*--------------------------------------------------------------------------*/
/* Common */
#define CHECK_ID_(name, id)     if ((id < 0) || ( name ## _ID_MAX <= id)) return E_ID;
#define CHECK_ID_TASK_(id)      CHECK_ID_(TASK, id)
#define CHECK_ID_FLAG_(id)      CHECK_ID_(FLAG, id)
#define CHECK_ID_MAILBOX_(id)   CHECK_ID_(MAILBOX, id)
/* Mail Box */
#define QUEUE_LENGTH    10
#define ITEM_SIZE       sizeof(void*)

/*--------------------------------------------------------------------------*/
/*  Global Variables                                                        */
/*--------------------------------------------------------------------------*/
/* Task */
static TaskHandle_t g_TaskHandles[TASK_ID_MAX];
/* Task Controll Block */
static StaticTask_t g_TCBs[TASK_ID_MAX];
/* Event Flag */
static EventGroupHandle_t   g_FlagHandles[FLAG_ID_MAX];
static StaticEventGroup_t   g_FlagGroups[FLAG_ID_MAX];
/* Mail Box */
static QueueHandle_t        g_MailBoxHandles[MAILBOX_ID_MAX];
static StaticQueue_t        g_MailBoxQueues[MAILBOX_ID_MAX];
static uint8_t              g_QueueStorageArea[MAILBOX_ID_MAX][QUEUE_LENGTH * ITEM_SIZE];

/*--------------------------------------------------------------------------*/
/*  APIs                                                                    */
/*--------------------------------------------------------------------------*/

ER cre_tsk(ID tskid, T_CTSK* pk_ctsk)
{
    if ((tskid < 0) || (TASK_ID_MAX <= tskid)) return E_ID;

    g_TaskHandles[tskid] = xTaskCreateStatic(
        pk_ctsk->task, pk_ctsk->tskatr, pk_ctsk->depth, NULL, pk_ctsk->itskpri, pk_ctsk->stk, &(g_TCBs[tskid]));
    return E_OK;
}

ER dly_tsk(RELTIM dlytim)
{
    TickType_t xCycleFrequency = pdMS_TO_TICKS((unsigned long int)dlytim);
    vTaskDelay(xCycleFrequency);
    return E_OK;
}

ER sus_tsk(ID tskid)
{
    CHECK_ID_TASK_(tskid);
    vTaskSuspend(g_TaskHandles[tskid]);
    return E_OK;
}

ER rsm_tsk(ID tskid)
{
    CHECK_ID_TASK_(tskid);
    vTaskResume(g_TaskHandles[tskid]);
    return E_OK;
}

ER cre_flg(ID flgid, T_CFLG* pk_cflg)
{
    if ((flgid < 0) || (FLAG_ID_MAX <= flgid)) return E_ID;

    (void)pk_cflg;
    g_FlagHandles[flgid] = xEventGroupCreateStatic(&(g_FlagGroups[flgid]));
    return E_OK;
}

ER set_flg(ID flgid, FLGPTN setptn)
{
    if ((flgid < 0) || (FLAG_ID_MAX <= flgid)) return E_ID;

    xEventGroupSetBits(g_FlagHandles[flgid], setptn);
    return E_OK;
}

ER wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN* p_flgptn)
{
    if ((flgid < 0) || (FLAG_ID_MAX <= flgid)) return E_ID;

    *p_flgptn = xEventGroupWaitBits(g_FlagHandles[flgid], waiptn,
        pdTRUE,     /* Clear Bits */
        (const BaseType_t)(wfmode == TWF_ANDW), portMAX_DELAY);
    return E_OK;
}

ER cre_mbx(ID mbxid, T_CMBX* pk_cmbx)
{
    CHECK_ID_MAILBOX_(mbxid);
    (void)pk_cmbx;

    /* void*�^��QUEUE_LENGTH�����L���[�C���O�\ */
    g_MailBoxHandles[mbxid] = xQueueCreateStatic(QUEUE_LENGTH,
        ITEM_SIZE,
        g_QueueStorageArea[mbxid],
        &(g_MailBoxQueues[mbxid]));

    return E_OK;
}

ER snd_mbx(ID mbxid, T_MSG* pk_msg)
{
    CHECK_ID_MAILBOX_(mbxid);

    pk_msg->msgqueue = (void*)pk_msg;
    xQueueSend(g_MailBoxHandles[mbxid], (void*)pk_msg, (TickType_t)0);

    return E_OK;
}

ER rcv_mbx(ID mbxid, T_MSG** ppk_msg)
{
    CHECK_ID_MAILBOX_(mbxid);

    T_MSG* pxRxedMessage;

    xQueueReceive(g_MailBoxHandles[0], &(pxRxedMessage), portMAX_DELAY);
    *ppk_msg = pxRxedMessage;

    return E_OK;
}