/*
    can接收中断函数，直接从官方代码改，去掉了不需要的功能函数
    其中包含本人引以为傲的改进版计圈，不可能漏记，不可能
    3508巅峰16000电流无负载计圈测试10分钟无一漏计，如有错误大概率是出现机械打滑
*/

#include "CAN_receive.h"
#include "can.h"
#include "remote_control.h"
#include "DMPower.h"

motor_measure_t motor[9];
sensor_measure_t lift_sensor[2];
const RC_ctrl_t *c_rc_data;
int16_t rc_flag = 0;
reset_t Reset_s;
Top_send_t  top_send;
damiao_maesure_t MOTOR1_t,MOTOR2_t,damiao_motor[2]; //定义一个一个一个测试达妙

#define get_motor_measure(ptr, data)                                    \
    {                                                                   \
        (ptr)->last_ecd = (ptr)->ecd;                                   \
        (ptr)->ecd = (uint16_t)((data)[0] << 8 | (data)[1]);            \
        (ptr)->speed_rpm = (int16_t)((data)[2] << 8 | (data)[3]);       \
        (ptr)->given_current = (uint16_t)((data)[4] << 8 | (data)[5]);  \
        (ptr)->temperate = (data)[6];                                   \
    }

#define get_sensor_measure(ptr, data)                                   \
    {                                                                   \
        (ptr)->signal_strength = (uint16_t)((data)[4] << 8 | (data)[5]);\
        (ptr)->dis_status = (uint8_t)(data)[3];                         \
        (ptr)->dis2 = (uint16_t)((data)[2] << 8 | (data)[1]);           \
        (ptr)->dis1 = (uint8_t)(data)[0];                               \
        (ptr)->dis0 = (ptr)->dis2 * 256 + (ptr)->dis1;                  \
        (ptr)->dis = (ptr)->dis0*1.000/1000;                            \
    }


//达妙一拖四接收
#define get_damiao_motor_measure(ptr, data)                                    \
    {                                                                   \
        (ptr)->last_ecd = (ptr)->ecd;                                   \
        (ptr)->ecd = (uint16_t)((data)[0] << 8 | (data)[1]);            \
        (ptr)->speed_rpm = (int16_t)((data)[2] << 8 | (data)[3]);       \
        (ptr)->given_current = (uint16_t)((data)[4] << 8 | (data)[5]);  \
        (ptr)->temperate = (data)[6];                                   \
        (ptr)->pcb_temperate = (data)[7];                                   \
    }
    

const motor_measure_t *get_motor_measure_point(uint8_t i)
{
    return &motor[i];
}

const reset_t *get_reset_point(void)
{
    return &Reset_s;
}

const sensor_measure_t *get_sensor_measure_point(uint8_t i)
{
    return &lift_sensor[i];
}
//达妙数据接收指针
const damiao_maesure_t *get_damiao_motor_measure_point(uint8_t i)
{
    return &damiao_motor[i];
}

void can_receive_init(void)
{
    for(int i=0;i<8;i++)
    {
        motor[i].round = 0;
    }
}

void receive_rc_board_com(uint8_t data[8])
{
    top_send.s0 = (int16_t)(data[0] << 8 | data[1]);
    top_send.s1 = (int16_t)(data[2] << 8 | data[3]);
    top_send.ch_3 = (int16_t)(data[4] << 8 | data[5]);
    top_send.v = (uint16_t)(data[6] << 8 | data[7]);
}

void receive_ss_board_com(uint8_t data[8])
{
    top_send.s0 = data[0];
    top_send.s1 = data[1];
    top_send.ch_1 = (int16_t)(data[2] << 8 | data[3]);
    // lift.pump_state = data[4];    //TODO气泵开关数据还未定义
}
//addmotor04
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if(Reset_s.Reset_last_key != Reset_s.Reset_key && Reset_s.Reset_last_key == 1)
    {
        for(int i=0;i<8;i++)
        {
            motor[i].round = 0;
        }
    }

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
    if(hcan == &hcan1)
    {
        switch (rx_header.StdId)
        {
            case CAN_RC_BOARM_COM_ID:
            {
                receive_rc_board_com(rx_data);
                break;
            }

            case CAN_LIFT_LEFT_ID:
            {
                static uint8_t i = 0;
                //get motor id
                i = rx_header.StdId - CAN_LIFT_LEFT_ID + 4;
                get_motor_measure(&motor[i], rx_data);
                if(motor[i].ecd - motor[i].last_ecd > 5000)
                {
                    motor[i].round--;
                }
                if(motor[i].ecd - motor[i].last_ecd < -5000)
                {
                    motor[i].round++;
                }
                break;
            }
            case CAN_LIFT_RIGHT_ID: //顺延至下面的可能的进程
            case CAN_ORE_LEFT_ID:
            case CAN_ORE_RIGHT_ID:
            {
                static uint8_t i = 0;
                //get motor id
                i = rx_header.StdId - CAN_LIFT_LEFT_ID + 4;
                get_motor_measure(&motor[i], rx_data);
                break;
            }
            
            
            default:
            {
                break;
            }
        
        }
        
    }
    if(hcan == &hcan2)
    {
        switch (rx_header.StdId)
        {
            case CAN_STRETCH_L_ID:
            {
                static uint8_t i = 0;
                //get motor id
                i = rx_header.StdId - CAN_ROLL_ID;
                get_motor_measure(&motor[i], rx_data);
                if(motor[i].ecd - motor[i].last_ecd > 5000)
                {
                    motor[i].round--;
                }
                if(motor[i].ecd - motor[i].last_ecd < -5000)
                {
                    motor[i].round++;
                }
            }
            
            case CAN_STRETCH_R_ID:
            {
                static uint8_t i = 0;
                //get motor id
                i = rx_header.StdId - CAN_ROLL_ID;
                get_motor_measure(&motor[i], rx_data);
                if(motor[i].ecd - motor[i].last_ecd > 5000)
                {
                    motor[i].round--;
                }
                if(motor[i].ecd - motor[i].last_ecd < -5000)
                {
                    motor[i].round++;
                }
            }
            case CAN_ROLL_ID:
            {
                static uint8_t i = 0;
                //get motor id
                i = rx_header.StdId - CAN_ROLL_ID;
                get_motor_measure(&motor[i], rx_data);
                if(motor[i].ecd - motor[i].last_ecd > 5000)
                {
                    motor[i].round--;
                }
                if(motor[i].ecd - motor[i].last_ecd < -5000)
                {
                    motor[i].round++;
                }
            }
            case CAN_SLID_ID:
            {
                static uint8_t i = 0;
                //get motor id
                i = rx_header.StdId - CAN_ROLL_ID;
                get_motor_measure(&motor[i], rx_data);
                if(motor[i].ecd - motor[i].last_ecd > 5000)
                {
                    motor[i].round--;
                }
                if(motor[i].ecd - motor[i].last_ecd < -5000)
                {
                    motor[i].round++;
                }
            }
            case CAN_YAW_ID:
            {
                static uint8_t i = 0;
                //get motor id
                i = rx_header.StdId - CAN_ROLL_ID;
                get_motor_measure(&motor[i], rx_data);
                if(motor[i].ecd - motor[i].last_ecd > 5000)
                {
                    motor[i].round--;
                }
                if(motor[i].ecd - motor[i].last_ecd < -5000)
                {
                    motor[i].round++;
                }
            }
            default:
            {
                break;
            }
        }  
    }
}
