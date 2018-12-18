#ifndef PID_H
#define PID_H



typedef struct _PIDdata {

    volatile float setpoint;

    // PID factors
    volatile float Kp;
    volatile float Ki;
    volatile float Kd;

    // PID terms
    volatile float Perr;
    volatile float Ierr;
    volatile float Derr;

    // PID terms limits
    volatile float Perrmin;
    volatile float Perrmax;
    volatile float Ierrmin;
    volatile float Ierrmax;
} PIDdata;
typedef PIDdata *ptrPIDdata;


void PID_Init(ptrPIDdata pPd);
void PID_SetPID(ptrPIDdata pPd, float setpoint, float pidP, float pidI, float pidD);
void PID_SetLimitsPerr(ptrPIDdata pPd, float Perr_min, float Perr_max);
void PID_SetLimitsIerr(ptrPIDdata pPd, float Ierr_min, float Ierr_max);
void PID_ResetIerr(ptrPIDdata pPd);
float PID_Update(ptrPIDdata pPd, float input);


#endif /* PID_H */
