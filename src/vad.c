#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
    "UNDEF", "MV", "MS", "S", "V", "INIT"};

const char *state2str(VAD_STATE st)
{
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct
{
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

//Funció que cridem a l'hora d'actualitzar l'STATE
Features compute_features(const float *x, int N, float fm)
{
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */

  Features feat;

  feat.zcr = compute_zcr(x, N, fm);
  feat.p = compute_power(x, N);
  feat.am = compute_am(x, N);

  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA *vad_open(float rate)
{
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data)
{
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data)
{
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */
double k0 = 0.0;
double k1, k2, a1, a2;
const int Ninit = 10;
const int NV = 1;
const int NS = 12;
int n = 0;
int ns = 0;
int nv = 0;


VAD_STATE vad(VAD_DATA *vad_data, float *x)
{

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length, vad_data->sampling_rate);

  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state)
  {
  case ST_INIT:

    //Càlcul de k0
    if (n < Ninit)
    {
      n++;
      k0 = k0 + pow(10, f.p / 100);
    }
    else
    {
      k0 = 100 * log10(k0 / Ninit);
      a1 = -0.08 * k0 + 2.0;
      //a2 = 5.0;
      a2 = -0.036 * a1 + 1.8;
      k1 = k0 + a1;
      k2 = k1 + a2;
      vad_data->state = ST_SILENCE;
    }
    break;

  case ST_SILENCE:
    ns = 0;
    nv = 0;

    //Si P(dB) > k1 pasamos a MV
    if (f.p > k1)
      vad_data->state = ST_MAYBE_VOICE;
    break;

  case ST_VOICE:
    ns = 0;
    nv = 0;
    if (f.p < k1)
      vad_data->state = ST_MAYBE_SILENCE;
    break;

  case ST_MAYBE_VOICE:
    if (nv < NV)
    {
      nv++;
    }
    else
    {
      if (f.p < k2)
      {
        vad_data->state = ST_SILENCE;
      }
      else
      {
        vad_data->state = ST_VOICE;
      }
    }
    break;

  case ST_MAYBE_SILENCE:
    if (ns < NS)
    {
      ns++;
    }
    else
    {
      if (f.p < k1)
      {
        vad_data->state = ST_SILENCE;
      }
      else
      {
        vad_data->state = ST_VOICE;
      }
    }
    break;

  case ST_UNDEF:
    break;
  }

  //Retornem l'STATE Actual
  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE ||
      vad_data->state == ST_MAYBE_SILENCE ||
      vad_data->state == ST_MAYBE_VOICE)
    return vad_data->state;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out)
{
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
