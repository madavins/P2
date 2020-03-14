#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sndfile.h>

#include "vad.h"
#include "vad_docopt.h"
#include "pav_analysis.h"

#define DEBUG_VAD 0x1

int main(int argc, char *argv[])
{
  int verbose = 0; /* To show internal state of vad: verbose = DEBUG_VAD; */

  SNDFILE *sndfile_in, *sndfile_out = 0;
  SF_INFO sf_info;
  FILE *vadfile;
  int n_read = 0, i;
  FILE *test;

  VAD_DATA *vad_data;                                  /* STATE, sampling_rate, frame_length i last_feature */
  VAD_STATE state, last_state, state_ini = ST_SILENCE; /* Constants referides a tots els possibles ESTATS */

  float *buffer, *buffer_zeros;
  int frame_size;         /* in samples */
  float frame_duration;   /* in seconds */
  unsigned int t, last_t; /* in frames */

  char *input_wav, *output_vad, *output_wav;

  DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "2.0");

  verbose = args.verbose ? DEBUG_VAD : 0;
  input_wav = args.input_wav;
  output_vad = args.output_vad;
  output_wav = args.output_wav;

  if (input_wav == 0 || output_vad == 0)
  {
    fprintf(stderr, "%s\n", args.usage_pattern);
    return -1;
  }

  /* Open input sound file */
  if ((sndfile_in = sf_open(input_wav, SFM_READ, &sf_info)) == 0)
  {
    fprintf(stderr, "Error opening input file %s (%s)\n", input_wav, strerror(errno));
    return -1;
  }

  if (sf_info.channels != 1)
  {
    fprintf(stderr, "Error: the input file has to be mono: %s\n", input_wav);
    return -2;
  }

  /* Open vad file */
  if ((vadfile = fopen(output_vad, "wt")) == 0)
  {
    fprintf(stderr, "Error opening output vad file %s (%s)\n", output_vad, strerror(errno));
    return -1;
  }

  if ((test = fopen("test.txt", "wt")) == NULL)
  {
    fprintf(stderr, "Error al abrir %s (%s)\n", "test.txt", strerror(errno));
    return -1;
  }

  /* Open output sound file, with same format, channels, etc. than input */
  if (output_wav)
  {
    if ((sndfile_out = sf_open(output_wav, SFM_WRITE, &sf_info)) == 0)
    {
      fprintf(stderr, "Error opening output wav file %s (%s)\n", output_wav, strerror(errno));
      return -1;
    }
  }

  vad_data = vad_open(sf_info.samplerate); //Asigna STATE = ST_INIT
  /* Allocate memory for buffers */
  frame_size = vad_frame_size(vad_data);
  buffer = (float *)malloc(frame_size * sizeof(float));
  buffer_zeros = (float *)malloc(frame_size * sizeof(float));
  for (i = 0; i < frame_size; ++i)
    buffer_zeros[i] = 0.0F;

  /************************/
  /*      IMPORTANT       */
  /************************/

  //Calculem durada de cada frame
  frame_duration = (float)frame_size / (float)sf_info.samplerate;
  //Establim estat inicial
  last_state = ST_UNDEF; //Canviar a S_INIT ???

  for (t = last_t = 0;; t++)
  { /* For each frame ... */
    /* End loop when file has finished (or there is an error) */
    if ((n_read = sf_read_float(sndfile_in, buffer, frame_size)) != frame_size)
      break;

    if (sndfile_out != 0)
    {
      /* TODO: copy all the samples into sndfile_out */
      //sf_count_t	sf_write_float	(SNDFILE *sndfile, const float *ptr, sf_count_t items) ;
      sf_write_float(sndfile_out, buffer, frame_size);
    }

    state = vad(vad_data, buffer); //Retorna state actual
    vad_show_state(vad_data, test);

    if (verbose & DEBUG_VAD)
      vad_show_state(vad_data, stdout);

    /* TODO: print only SILENCE and VOICE labels */
    /* As it is, it prints UNDEF segments but is should be merge to the proper value */

    if (state != last_state)
    {
      if (t != last_t)
      {
        if ((state_ini != state) && (state == ST_VOICE || state == ST_SILENCE) && (last_state == ST_MAYBE_SILENCE || last_state == ST_MAYBE_VOICE))
        {
          fprintf(vadfile, "%.5f\t%.5f\t%s\n", (last_t * frame_duration), ((t - 1) * frame_duration), state2str(state_ini));
          state_ini = state;
          last_t = t - 1;
        }
        last_state = state;
      }
    }

    if (sndfile_out != 0)
    {
      /* TODO: go back and write zeros in silence segments */
    }
  }

  //Hem acabat de llegir tots els Frames
  state = vad_close(vad_data); //Decidim que fem amb les últimes mostres restants
  /* TODO: what do you want to print, for last frames? */
  if (t != last_t)
  {
    fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration + n_read / (float)sf_info.samplerate, state2str(ST_SILENCE));
  }

  /* clean up: free memory, close open files */
  free(buffer);
  free(buffer_zeros);
  sf_close(sndfile_in);
  fclose(vadfile);
  fclose(test);
  if (sndfile_out)
    sf_close(sndfile_out);
  return 0;
}
