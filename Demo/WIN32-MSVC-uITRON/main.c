/*--------------------------------------------------------------------------*/
/*  Includes                                                                */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <stdint.h>

#include "portmacro.h"
#include "uITRON.h"
#include "UserDefinitions.h"

/* ��ނȂ�FreeRTOS I/F���g���ꍇ�A�蓮�Œǉ� */
void vTaskStartScheduler(void);

/*--------------------------------------------------------------------------*/
/*  Macro and Typedef Definitions                                           */
/*--------------------------------------------------------------------------*/
/*--- Message ---*/
typedef struct {
    T_MSG       msg;
    ID          id;
    uint32_t    params[1];
} MESSAGE_ENTITY;

#define MESSAGE_SIZE    (64)
static MESSAGE_ENTITY   g_Messages[MESSAGE_SIZE];
static uint16_t         g_MessageIndex = 0;

/* ���b�Z�[�W�擾 */
static MESSAGE_ENTITY* GetUserMessage(void) {
    uint16_t index = g_MessageIndex++;
    index %= MESSAGE_SIZE;
    return &(g_Messages[index]);
}
/* ����͂܂��p�ӂ��Ă��Ȃ� */

/*--- Event ---*/
#define EVENT_ALL       (0x00FFFFFF)    /* �ő��24-bit */
/* Reserved bit0 - bit15 */
#define EVENT_STOP      (0x00010000)
#define EVENT_RESTART   (0x00020000)

/*--- Task ---*/
#define CREATE_TASK(name, func) {   \
    T_CTSK ctsk_;    \
    ctsk_.tskatr = #name;   \
    ctsk_.task = func;   \
    ctsk_.itskpri = TASK_PRI(name); \
    ctsk_.stk = TASK_STACK(name);   \
    ctsk_.depth = TASK_STACK_DEPTH(name);   \
    cre_tsk(TASK_ID(name), &ctsk_); \
}

/*--------------------------------------------------------------------------*/
/*  Prototypes                                                              */
/*--------------------------------------------------------------------------*/
static void SendTask(void *);
static void RecvTask(void *);
static void TimerTask(void*);
static void CommandTask(void *);

/*--------------------------------------------------------------------------*/
/*  Static Functions                                                        */
/*--------------------------------------------------------------------------*/

static ER CreateOsResources(void)
{
    T_CFLG cflg;
    cre_flg(FLAG_ID(SEND), &cflg);

    T_CMBX cmbx;
    cre_mbx(MAILBOX_ID(RECV), &cmbx);

    return E_OK;
}

static ER CreateOsTasks(void)
{
    CREATE_TASK(SEND, SendTask);
    CREATE_TASK(RECV, RecvTask);

    CREATE_TASK(TIMER, TimerTask);

    CREATE_TASK(COMMAND, CommandTask);

    return E_OK;
}

static void PrintUsage()
{
    DEBUG_PRINT("");
    DEBUG_PRINT("uTITRON (based on FreeRTOS) Demo Program");
    DEBUG_PRINT("   1 - 15�̐��l    :   ���l���b�Z�[�W�̑���M");
    DEBUG_PRINT("   stop            :   ��M�^�X�N�̈ꎞ��~");
    DEBUG_PRINT("   start           :   ��M�^�X�N�̍ĊJ");
    DEBUG_PRINT("   exit            :   �f���̏I��");
    DEBUG_PRINT("   help            :   ���̃��b�Z�[�W��\��");
    DEBUG_PRINT("");
    DEBUG_PRINT("");
}

/*--------------------------------------------------------------------------*/
/*  Main and Task Functions                                                 */
/*--------------------------------------------------------------------------*/

int main(void)
{
    CreateOsResources();
    CreateOsTasks();

    /* �X�P�W���[�����O�J�n */
    /* �Ή�����uITRON�T�[�r�X�R�[���͂Ȃ��E�E�E */
    vTaskStartScheduler();

    /* �����ɂ͗��Ȃ��͂� */

    return 0;
}

static void SendTask(void* params)
{
    uint32_t count = 0;

    /* Just to remove compiler warning. */
    (void)params;

    while (1) {
        FLGPTN flgptn;
        wai_flg(FLAG_ID(SEND), EVENT_ALL, TWF_ORW, &flgptn);
        DEBUG_PRINT("[%s]: RECV EVENT (%08X)", __func__, flgptn);

        /* Suspend Task */
        if (flgptn & EVENT_STOP) {
            sus_tsk(TASK_ID(RECV));
            continue;
        }
        /* Resume Task */
        if (flgptn & EVENT_RESTART) {
            rsm_tsk(TASK_ID(RECV));
            continue;
        }

        MESSAGE_ENTITY* ent;
        ent = GetUserMessage();
        /* Numeric Event */
        /* ���b�Z�[�WID�̓C�x���g���i���l�ɖ߂��āj�ݒ� */
        /* �p�����[�^�ɃJ�E���^��n�� */
        int32_t num = 0;
        for (num = 0; !((flgptn >> num) & 1) ; num++) ;
        ent->id = (ID)num;
        ent->params[0] = ++count;

        snd_mbx(MAILBOX_ID(RECV), (T_MSG *)ent);
    }
}

static void RecvTask(void* params)
{
    /* Just to remove compiler warning. */
    (void)params;

    while (1) {
        T_MSG *msg;
        rcv_mbx(MAILBOX_ID(RECV), &msg);
        MESSAGE_ENTITY* ent = (MESSAGE_ENTITY*)(msg);
        DEBUG_PRINT("[%s]: RECV MESSAGE (%d,%d)", __func__, ent->id, ent->params[0]);
    }
}

static void TimerTask(void *params)
{
    /* Just to remove compiler warning. */
    (void)params;

    while (1) {
        RELTIM tim = 1000;
        DEBUG_PRINT("[%s]: DELAY %dms", __func__, tim);
        dly_tsk(tim);
    }
}

static void CommandTask(void* params)
{
    char str[256];

    /* Just to remove compiler warning. */
    (void)params;

    while (1) {

        /* gets_s()�ŉ��s���͂܂Ńu���b�N���Ă��܂����A�����RTOS�̂���@�ɔ����Ă���B */
        /* �i�{���̓C�x���g�҂��Ȃ�A���b�Z�[�W�҂��Ȃ�̃T�[�r�X�R�[�����ĂԂׂ��ł��낤�j */
        gets_s(str, sizeof(str));

        /* Command Parser */
        int32_t num = atol(str);
        /* Numeric Event (1 To 15) */
        if ((0 < num) && (num < 16)) {
            set_flg(FLAG_ID(SEND), (1 << num));
        }
#define EQUALS_(x) (0 == strcmp(str, #x))
        /* Suspend */
        else if (EQUALS_(stop)) {
            set_flg(FLAG_ID(SEND), EVENT_STOP);
        }
        /* Resume */
        else if (EQUALS_(start)) {
            set_flg(FLAG_ID(SEND), EVENT_RESTART);
        }
        /* Print Usage */
        else if (EQUALS_(help)) {
            sus_tsk(TASK_ID(TIMER));
            PrintUsage();
            rsm_tsk(TASK_ID(TIMER));
        }
        /* Exit */
        else if (EQUALS_(exit)) {
            /* �����I�� */
            exit(1);
        }
#undef EQUALS_
    }
}