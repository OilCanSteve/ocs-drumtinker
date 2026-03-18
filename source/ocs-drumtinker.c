// Copyright 2014-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "./uris.h"

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>




void debugMidiNote(LV2_Handle instance, LV2_Atom_Event * event, int8_t  msg[]);

enum { IN = 0, OUT_A = 1, OUT_B = 2, OUT_C= 3, OUT_D= 4, NOTESTART = 5, 
       VOL_A = 6, VOL_B = 7, VOL_C = 8, VOL_D = 9, 
       CHN_A = 10, CHN_B = 11, CHN_C = 12, CHN_D = 13}; 

typedef struct {
  // Features
  LV2_URID_Map*  map;
  LV2_Log_Logger logger;

  // Ports
  struct {
  const LV2_Atom_Sequence* in_port;
  LV2_Atom_Sequence*       midi_out[4];
  const float* noteStart;
  const float* defVolOut[4];
  const float* defChnOut[4];
  const float* note_n_vol_ptr[32];
  } ports;

  struct {
    uint8_t in_note;
    uint8_t out_idx;
    struct {
    LV2_Atom_Event event;
    uint8_t        msg[3];
  } MIDINoteEvent;
  } MidiNode[16];

  // URIs
  DrumtinkerURIs uris;
} Drumtinker;

static void
connect_port(LV2_Handle instance, uint32_t port, void* data)
{
  Drumtinker* self = (Drumtinker*)instance;
  //lv2_log_note(&self->logger, "Connect Port %d\n", port); 

  if (port==0)
  {
    self->ports.in_port = (const LV2_Atom_Sequence*)data;
    return;
  }
  if (port<5)
  {
    self->ports.midi_out[port-1] = (LV2_Atom_Sequence*)data;
    return;
  }
  if (port==5)
  {
    self->ports.noteStart = (const float*)data;
    return;
  }
  if (port<10)
  {
    self->ports.defVolOut[port-6] = (const float*)data;
    return;
  }
  if (port<14)
  {
    self->ports.defChnOut[port-10] = (const float*)data;
    return;
  }
  if (port<46)
  {
    self->ports.note_n_vol_ptr[port-14] = (const float*)data;
    return;
  }
   
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
  // Allocate and initialise instance structure.
  Drumtinker* self = (Drumtinker*)calloc(1, sizeof(Drumtinker));
  if (!self) {
    return NULL;
  }

  // Scan host features for URID map
  // clang-format off
  const char*  missing = lv2_features_query(
    features,
    LV2_LOG__log,  &self->logger.log, false,
    LV2_URID__map, &self->map,        true,
    NULL);
  // clang-format on

  lv2_log_logger_set_map(&self->logger, self->map);
  if (missing) {
    lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
    free(self);
    return NULL;
  }

  

  map_mididrums_uris(self->map, &self->uris);

  return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
  free(instance);
}

void parseNoteSettings (Drumtinker* self);

void parseNoteSettings (Drumtinker* self)
{
  for (int i=0; i<16; i++)
  {
    self->MidiNode[i].in_note = *self->ports.noteStart + i;
    self->MidiNode[i].MIDINoteEvent.event.time.frames = 0;
    self->MidiNode[i].MIDINoteEvent.event.body.type = self->uris.midi_Event;
    self->MidiNode[i].MIDINoteEvent.event.body.size = 3;
    self->MidiNode[i].out_idx = i/4;
    }
  }

static void
run(LV2_Handle instance, uint32_t sample_count)
{
  Drumtinker*     self = (Drumtinker*)instance;
  DrumtinkerURIs* uris = &self->uris;
  bool cleared[4] = {false, false, false, false}; //to keep track of which output ports have been cleared

  

  // Read incoming events
  LV2_ATOM_SEQUENCE_FOREACH (self->ports.in_port, ev) {
    if (ev->body.type == uris->midi_Event) {
      const uint8_t* const msg = (const uint8_t*)(ev + 1);
      switch (lv2_midi_message_type(msg)) {
      case LV2_MIDI_MSG_NOTE_ON:
      case LV2_MIDI_MSG_NOTE_OFF:
      
        //lv2_log_note(&self->logger, "Incoming Note %d\n", msg[1]);
        //if incoming note is either greater than the start note or less than the start note + 16 then it is not a note to be processed
        if (!(msg[1] >= (int) *self->ports.noteStart && msg[1] < (int) *self->ports.noteStart + 16))  return;
        //lv2_log_note(&self->logger, "Valid note %d\n", msg[1]);
        parseNoteSettings(self);
        //lv2_log_note(&self->logger, "Inputs parsed\n");
          for (int i=0;i<16;i++)
          {
            if (msg[1] == self->MidiNode[i].in_note)
            { 
              
              int outNote = (uint8_t)*self->ports.note_n_vol_ptr[i*2];
              int outVol = (uint8_t)*self->ports.note_n_vol_ptr[1+i*2];
              if (outVol ==-2 ) outVol = msg[2];  //uses the incoming note's volume
              else if (outVol==-1) //uses the default volume for that channel
              {
                outVol = (uint8_t) *self->ports.defVolOut[i/4];
              }
              // in cases of outVol>=0 then it just uses the specified value (from the UI)

               if (*self->ports.defChnOut[i/4]<0) self->MidiNode[i].MIDINoteEvent.msg[0] = msg[0];
               else self->MidiNode[i].MIDINoteEvent.msg[0] = (msg[0] & 0xF0) |  ((uint8_t) *self->ports.defChnOut[i/4] & 0x0F);
               self->MidiNode[i].MIDINoteEvent.msg[1] = outNote;
               self->MidiNode[i].MIDINoteEvent.msg[2] = outVol;
               uint32_t out_capacity = self->ports.midi_out[i/4]->atom.size;
               if (!cleared[i/4])
               {
                 lv2_atom_sequence_clear(self->ports.midi_out[i/4]);
                 cleared[i/4] = true;
               }
               //debugMidiNote(instance, &self->MidiNode[i].MIDINoteEvent.event, self->MidiNode[i].MIDINoteEvent.msg);
               self->ports.midi_out[i/4]->atom.type = self->uris.atom_Sequence;
               lv2_atom_sequence_append_event(self->ports.midi_out[i/4], out_capacity, &self->MidiNode[i].MIDINoteEvent.event);

          } //end of if note matches
        }//end for loop
       break; //out of noteOn/Off
      default:
      break;
      } //end of switch
    }//end of if midi event
  } //end of LV2_ATOM_SEQUENCE_FOREACH
}

/* void debugMidiNote(LV2_Handle instance, LV2_Atom_Event * event, int8_t  msg[])
{
  Drumtinker*     self = (Drumtinker*)instance;
  int8_t a = msg[0] & 15; 
  int8_t b = msg[0] >> 4;
  lv2_log_note(&self->logger, "------------------------------------------\n");
  lv2_log_note(&self->logger, "Frames <%ld>\n", (long) event->time.frames);
  lv2_log_note(&self->logger, "Body Type <%ld>\n", (long) event->body.type);
  lv2_log_note(&self->logger, "Body Size <%ld>\n",  (long)event->body.size);
  lv2_log_note(&self->logger, "Status A (Channel)<%X>\n", a);
  lv2_log_note(&self->logger, "Status B (Note On/Off) <%X>\n", b);
  lv2_log_note(&self->logger, "Status <%X>\n", msg[0]);
  lv2_log_note(&self->logger, "Key <%d>\n", msg[1]);
  lv2_log_note(&self->logger, "Velocity <%d>\n", msg[2]);
} */

static const void*
extension_data(const char* uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {DRUMTINKER_URI,
                                          instantiate,
                                          connect_port,
                                          NULL, // activate,
                                          run,
                                          NULL, // deactivate,
                                          cleanup,
                                          extension_data};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  return index == 0 ? &descriptor : NULL;
}