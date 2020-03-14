#include <math.h>
#include "pav_analysis.h"

float compute_power(const float *x, unsigned int N){

    float potencia = 0.0;

    //Calculamos el sumatorio de los valores al cuadrado de la señal
    for (int n = 0; n < N; n++){
        potencia += x[n]*x[n];
    }
    //Devolvemos la potencia de la señal en dB
    return 10*log10((1/(float)N)*potencia);
}

float compute_am(const float *x, unsigned int N){

    float amplitud = 0.0;

    //Calculamos primero la suma de valores absolutos
    for (int n = 0; n < N; n++){
        amplitud += fabs(x[n]);
    }

    return amplitud/N;
}

float compute_zcr(const float *x, unsigned int N, float fm){

    float zcr = 0.0;

    for (int n = 1; n < N; n++){
        if((x[n]*x[n-1]) < 0){
            zcr += 1;
        }
    }

    return (fm/2)*(1/((float)N-1))*zcr;
}

