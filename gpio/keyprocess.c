#include <stdio.h>
#include <time.h>
 
// 定义长按键的TICK数, 以及连发间隔的TICK数, 单位是毫秒ms
#define KEY_DEBOUNCE_PERIOD     20   // 延时消抖时间
#define KEY_LONG_PERIOD         1000
#define KEY_CONTINUE_PERIOD     200
 
#define TRUE    1
#define FALSE   0
 
typedef unsigned int Bool;
 
typedef enum {
    LOW = 0,
    HIGH
} KEY_LEVEL_E;
 
typedef enum
{
    KEY1 = 0,
    KEY2,
    KEY3,
    KEY4,
    KEY_MAX_NUM
} KEY_NUM_E;
 
// 定义按键返回值状态(按下,长按,连发,释放,无动作)
typedef enum {
    KEY_NULL = 0,
    KEY_DOWN,
    KEY_LONG,
    KEY_CONTINUE,
    KEY_UP
} KEY_VALUE_E;
 
typedef enum {
    KEY_STATE_INIT = 0,
    KEY_STATE_WOBBLE,
    KEY_STATE_PRESS,
    KEY_STATE_LONG,
    KEY_STATE_CONTINUE,
    KEY_STATE_RELEASE
} KEY_STATE_E;
 
typedef struct
{
    KEY_NUM_E eKeyId;
    KEY_LEVEL_E eKeyLevel;         //key level(low or high)
    unsigned int downTick;
    unsigned int upTick;
    KEY_STATE_E eKeyCurState;      //key cur state(fsm)
    KEY_STATE_E eKeyLastState;     //key last state(fsm)
    Bool bStateChangedFlag;        //state changed flag
    KEY_VALUE_E eLastKeyValue;     //key value
} KEY_HANDLE_T;
 
typedef void (*pf)(void);
 
typedef struct
{
    pf keyDownAction;
    pf keyLongAction;
    pf keyContinueAction;
    pf keyUpAction;
} KEY_FUNCTION_T;
 
KEY_HANDLE_T keyList[KEY_MAX_NUM] =
{
    {KEY1, LOW, 0, 0, KEY_STATE_INIT, KEY_STATE_INIT, FALSE, KEY_NULL},
    {KEY2, LOW, 0, 0, KEY_STATE_INIT, KEY_STATE_INIT, FALSE, KEY_NULL},
    {KEY3, LOW, 0, 0, KEY_STATE_INIT, KEY_STATE_INIT, FALSE, KEY_NULL},
    {KEY4, LOW, 0, 0, KEY_STATE_INIT, KEY_STATE_INIT, FALSE, KEY_NULL}
};
 
 
// 用于获取当前时间, 单位是毫秒ms
unsigned int getTime0(void)
{
    struct timespec time;
 
    clock_gettime(CLOCK_MONOTONIC, &time);
 
    return ((time.tv_sec * 1000) + (time.tv_nsec/1000000));
}
 
unsigned int timeDiffFromNow(unsigned int time0)
{
    unsigned int cur = 0;
    struct timespec time;
 
    clock_gettime(CLOCK_MONOTONIC, &time);
 
    cur = (time.tv_sec * 1000) + (time.tv_nsec/1000000);
 
    return cur - time0;
}
 
void keyScan(KEY_HANDLE_T* key)
{
    // gpioReadPin()需根据你的平台来写
    if(HIGH == gpioReadPin(key->eKeyId))
    {
        // 根据原理图, HIGH代表松开
        key->eKeyLevel = HIGH;
    }
    else
    {
        // 根据原理图, LOW代表按下
        key->eKeyLevel = LOW;
    }
 
    // 获取当前key是按下还是松开
    KEY_LEVEL_E level = key->eKeyLevel;
 
    key->eLastKeyValue = KEY_NULL;
 
    switch(key->eKeyCurState)
    {
        case KEY_STATE_INIT:
            if(LOW == level)
            {
                key->downTick = getTime0();
 
                key->bStateChangedFlag = TRUE;
                key->eKeyCurState = KEY_STATE_WOBBLE;
            }
            break;
 
        case KEY_STATE_WOBBLE:
            if(LOW == level)
            {
                if(timeDiffFromNow(key->downTick) >= KEY_DEBOUNCE_PERIOD)
                {
                    key->bStateChangedFlag = TRUE;
                    key->eKeyCurState = KEY_STATE_PRESS;
                }
            }
            else
            {
                key->bStateChangedFlag = TRUE;
                key->eKeyCurState = KEY_STATE_INIT;
            }
            break;
 
        case KEY_STATE_PRESS:
            if(TRUE == key->bStateChangedFlag)
            {
                key->bStateChangedFlag = FALSE;
                key->eLastKeyValue = KEY_DOWN;
            }
 
            if(LOW == level)
            {
                if(timeDiffFromNow(key->downTick) >= KEY_LONG_PERIOD)
                {
                    key->bStateChangedFlag = TRUE;
                    key->eKeyCurState = KEY_STATE_LONG;
                }
            }
            else
            {
                key->upTick = getTime0();
 
                key->bStateChangedFlag = TRUE;
                key->eKeyCurState = KEY_STATE_RELEASE;
            }
            break;
 
        case KEY_STATE_LONG:
            if(TRUE == key->bStateChangedFlag)
            {
                key->bStateChangedFlag = FALSE;
                key->eLastKeyValue = KEY_LONG;
            }
 
            if(LOW == level)
            {
                if(timeDiffFromNow(key->downTick) >= (KEY_LONG_PERIOD + KEY_CONTINUE_PERIOD))
                {
                    key->downTick = getTime0();
 
                    key->bStateChangedFlag = TRUE;
                    key->eKeyCurState = KEY_STATE_CONTINUE;
                }
            }
            else
            {
                key->upTick = getTime0();
 
                key->bStateChangedFlag = TRUE;
                key->eKeyCurState = KEY_STATE_RELEASE;
            }
            break;
 
        case KEY_STATE_CONTINUE:
            if(TRUE == key->bStateChangedFlag)
            {
                key->bStateChangedFlag = FALSE;
                key->eLastKeyValue = KEY_CONTINUE;
            }
 
            if(LOW == level)
            {
                if(timeDiffFromNow(key->downTick) >= KEY_CONTINUE_PERIOD)
                {
                    key->downTick = getTime0();
 
                    key->bStateChangedFlag = TRUE;
                    key->eKeyCurState = KEY_STATE_CONTINUE;
                }
            }
            else
            {
                key->upTick = getTime0();
 
                key->bStateChangedFlag = TRUE;
                key->eKeyCurState = KEY_STATE_RELEASE;
            }
            break;
 
        case KEY_STATE_RELEASE:
            if(timeDiffFromNow(key->upTick) >= KEY_DEBOUNCE_PERIOD) // 松开按键也来个消抖
            {
                if(TRUE == key->bStateChangedFlag)
                {
                    key->bStateChangedFlag = FALSE;
                    key->eLastKeyValue = KEY_UP;
                }
 
                key->bStateChangedFlag = TRUE;
                key->eKeyCurState = KEY_STATE_INIT;
            }
            else
            {
                if(LOW == level)
                {
                    key->eKeyCurState = key->eKeyLastState;
                }
            }
            break;
 
        default:
            break;
    }
 
    key->eKeyLastState = key->eKeyCurState;
}
 
void key1DownAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key1LongAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key1ContinueAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key1UpAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key2DownAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key2LongAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key2ContinueAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key2UpAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key3DownAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key3LongAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key3ContinueAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key3UpAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key4DownAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key4LongAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key4ContinueAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
void key4UpAction(void)
{
    printf("%s\n", __FUNCTION__);
}
 
KEY_FUNCTION_T keyMapList[KEY_MAX_NUM] =
{
    {key1DownAction, key1LongAction, key1ContinueAction, key1UpAction}, //KEY1
    {key2DownAction, key2LongAction, key2ContinueAction, key2UpAction}, //KEY2
    {key3DownAction, key3LongAction, key3ContinueAction, key3UpAction}, //KEY3
    {key4DownAction, key4LongAction, key4ContinueAction, key4UpAction}, //KEY4
};
 
void keyMapHandle(KEY_HANDLE_T *key, KEY_FUNCTION_T *keyMap)
{
    unsigned char keyValue = key->eLastKeyValue;
 
    if(KEY_NULL == keyValue)
    {
        return;
    }
    else
    {
        switch(keyValue)
        {
            case KEY_DOWN:
                if(keyMap->keyDownAction != NULL)
                {
                    keyMap->keyDownAction();
                }
                break;
 
            case KEY_LONG:
                if(keyMap->keyLongAction != NULL)
                {
                    keyMap->keyLongAction();
                }
                break;
 
            case KEY_CONTINUE:
                if(keyMap->keyContinueAction != NULL)
                {
                    keyMap->keyContinueAction();
                }
                break;
 
            case KEY_UP:
                if(keyMap->keyUpAction != NULL)
                {
                    keyMap->keyUpAction();
                }
                break;
 
            default:
                break;
        }
    }
}
 
int main(void)
{
    KEY_NUM_E keyId = KEY_MAX_NUM;
 
    while(1)
    {
        for(keyId=KEY1; keyId<KEY_MAX_NUM; keyId++)
        {
            keyScan(&keyList[keyId]);
        }
 
        for(keyId=KEY1; keyId<KEY_MAX_NUM; keyId++)
        {
            keyMapHandle(&keyList[keyId], &keyMapList[keyId]);
        }
 
        usleep(10*1000);
    }
 
    return 0;
}