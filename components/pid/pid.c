#include "pid.h"


PIDdata pid_defaults = {
    .setpoint = 1234.0f,

    .kP = 123.4f,
    .kI = 12.34f,
    .kD = 1.234f,

    .Perrmin = -12345.0f,
    .Perrmax = 12345.0f,
    .Ierrmin = -54321.0f,
    .Ierrmax = 54321.0f
};
ptrPIDdata p_pid_defaults = &pid_defaults;

PIDdata pid_data;
ptrPIDdata p_pid_data = &pid_data;


void PID_Init(ptrPIDdata pPd) {

    // pPd->_input_prev = 0.0f;

    // pPd->setpoint = 0.0f;

    // pPd->kP = 0.0f;
    // pPd->kI = 0.0f;
    // pPd->kD = 0.0f;

    // pPd->Perr = 0.0f;
    // pPd->Ierr = 0.0f;
    // pPd->Derr = 0.0f;

    // pPd->Perrmin = -1000.0f;
    // pPd->Perrmax = 1000.0f;
    // pPd->Ierrmin = -1000.0f;
    // pPd->Ierrmax = 1000.0f;

    memcpy(pPd, p_pid_defaults, sizeof(PIDdata));
}

/*
 *  Set coefficients
 */
void PID_Coefficients(ptrPIDdata pPd, float setpoint, float kP, float kI, float kD) {

    pPd->setpoint = setpoint;

    pPd->kP = kP;
    pPd->kI = kI;
    pPd->kD = kD;
}


/*
 *  Set proportional term limits
 */
void PID_SetLimitsPerr(ptrPIDdata pPd, float Perr_min, float Perr_max) {
    pPd->Perrmin = Perr_min;
    pPd->Perrmax = Perr_max;
}


/*
 *  Set integral term limits
 */
void PID_SetLimitsIerr(ptrPIDdata pPd, float Ierr_min, float Ierr_max) {
    pPd->Ierrmin = Ierr_min;
    pPd->Ierrmax = Ierr_max;
}


/*
 *  Reset integral term accumulated error
 */
void PID_ResetIerr(ptrPIDdata pPd) {
    pPd->Ierr = 0.0f;
}


/*
 *  PID control algorithm. If this function get called always at the same period, dt=1 can be used,
 *  otherwise it should be calculated
 */
float PID_Update(ptrPIDdata pPd, float input) {

    // compute P error
    pPd->Perr = pPd->setpoint - input;
    if (pPd->Perr < pPd->Perrmin) {
        pPd->Perr = pPd->Perrmin;
    }
    else if (pPd->Perr > pPd->Perrmax) {
        pPd->Perr = pPd->Perrmax;
    }

    // compute I error
    pPd->Ierr += pPd->Perr;
    if (pPd->Ierr < pPd->Ierrmin) {
        pPd->Ierr = pPd->Ierrmin;
    }
    else if (pPd->Ierr > pPd->Ierrmax) {
        pPd->Ierr = pPd->Ierrmax;
    }

    // compute D error
    pPd->Derr = pPd->_input_prev - input;

    // record last value
    pPd->_input_prev = input;

    return ((pPd->kP * pPd->Perr) + (pPd->kI * pPd->Ierr) + (pPd->kD * pPd->Derr));
}
