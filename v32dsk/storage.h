#ifndef _STORAGE_H
#define _STORAGE_H

//////////////////////////////////////////////////////////////////////////////
//
// v32tools/v32dsk library: "storage.h"          File version: 2026/01/31
// ----------------------------------------------------------------------
// This file contains all definitions and functions needed by programs to
// access data from a externally-monitored memory card, allowing for data
// to be read in and written to it from Vircon32 programs.  The aim is to
// provide access to storage well in excess of that of a single MEM-CARD.
//
// This includes global v32dsk parameters,  as well as specific functions
// functions to read and write the contents of the memory card.
//
// It works by deploying a designated word as a set of status flags  that
// can help communicate with the outside system.  This word will  then be
// checked by both sides (Vircon32 and outside world):
//
//   * lock status:
//     > 0: no claim
//     > 1: Vircon32 access lock
//     > 2: outside world access lock
//
// If the lock status is non-zero, either side needs to take care not  to
// perform any reads or writes until the lock is cleared.
//
// For timing purposes, transactions will be  calibrated  for  individual
// frames.
//
// PORT: 0x600 -> MEM_Connected
//
//////////////////////////////////////////////////////////////////////////////

#include "memcard.h"

// bool card_is_connected();
// void card_read_signature( game_signature* signature );
// void card_write_signature( game_signature* signature );
// bool card_signature_matches( game_signature* expected_signature );
// bool card_is_empty();
// void card_read_data( void* destination, int offset_in_card, int size );
// void card_write_data( void* source, int offset_in_card, int size );
