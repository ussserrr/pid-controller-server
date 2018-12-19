#ifndef PID_H
#define PID_H


#include <string.h>


typedef struct _PIDdata {

    float _input_prev;

    float setpoint;

    // PID factors
    float kP;
    float kI;
    float kD;

    // PID terms
    float Perr;
    float Ierr;
    float Derr;

    // PID terms limits
    float Perrmin;
    float Perrmax;
    float Ierrmin;
    float Ierrmax;
} PIDdata;
typedef PIDdata *ptrPIDdata;


// extern PIDdata pid_defaults;
// extern ptrPIDdata p_pid_defaults;

extern PIDdata pid_data;
extern ptrPIDdata p_pid_data;


void PID_Init(ptrPIDdata pPd);
void PID_Coefficients(ptrPIDdata pPd, float setpoint, float kP, float kI, float kD);
void PID_SetLimitsPerr(ptrPIDdata pPd, float Perr_min, float Perr_max);
void PID_SetLimitsIerr(ptrPIDdata pPd, float Ierr_min, float Ierr_max);
void PID_ResetIerr(ptrPIDdata pPd);
float PID_Update(ptrPIDdata pPd, float input);


#endif /* PID_H */
