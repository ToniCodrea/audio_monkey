#pragma once
#ifndef _SOUND_H_
#define _SOUND_H_

#define FREQ 44100   // Sample rate
#define CAP_SIZE 512 // How much to capture at a time (affects latency)
#define SETUP_TIME 3 // Setup time for mic volume

extern void setup_quiet(void);
extern void setup_neutral(void);
extern void setup_quiet_db(void);
extern void setup_neutral_db(void);
extern void init_mic(void);
extern void close_mic(void);
// gets normalized level of height to add to platform, between 0 and 100
extern char get_level(void);
extern int get_quiet_amplitude(void);
extern int get_neutral_amplitude(void);

extern int get_max_amplitude(void);
#endif