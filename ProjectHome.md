The hardware for nrepeater consist of a HAM radio receiver, and a transmitter, both units' control ports are connected to the PC's parallel port and to the sound card.
When the receiver opens, the app starts recording on the sound card and simultaneously turns on the transmitter and plays back the recorded audio.

nrepeater supports parrot mode: the incoming audio is stored in the PC's memory and played back when receiving is finished. This mode needs only one transceiver hooked up to the computer.

Requires:
  * libparapin (communicating with the receiver & transmitter using the parallel port)
  * libsndfile (for reading wav files)
  * libogg (for archiving)
  * libspeex (for archiving)
  * libsamplerate (for Speex, DTMF decoding)
  * libflite (for speech synthetizing)

Features:
  * relay and parrot repeater modes
  * playing roger beep WAV file after receiving finished (delay is adjustable)
  * traffic logging in both text and compressed audio file (using Speex encoding)
  * built-in software audio level compression with adjustable ratio, threshold, lookahead and attack/hold/release times
  * DTMF decoder, with customizable actions for each decoded sequence: log, shell exec, PC speaker beep, switch parrot mode on/off, play wav file, play random wav file from a directory, read text using speech synthetizer
  * playing ack/fail beep WAV file after DTMF sequence received and the associated action is finished
  * LITZ emergency alert support: LITZ is a new system promoted by the ARRL to provide a means for a repeater user to request emergency assistance without being familiar with the operation of the repeater. If a repeater user transmits a DTMF 0 for three seconds, nrepeater will alert the control operators
  * nrepeater can say the current time, date and everything else using the built-in speech synthetizer
  * lots of configuration options, but easy to setup

_Future plans:_
  * connecting to existing networks such as EQSO


# **The project is in an early, alpha state, without documentation.** #